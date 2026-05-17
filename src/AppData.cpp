#include "AppData.h"
#include <QFile>
#include <QDateTime>
#include <cstring>

// ── CRC32 (IEEE 802.3) ────────────────────────────────────────────────────────
uint32_t AppData::crc32(const uint8_t* data, int len)
{
    uint32_t crc = 0xFFFFFFFFu;
    for (int i = 0; i < len; ++i) {
        crc ^= data[i];
        for (int b = 0; b < 8; ++b)
            crc = (crc >> 1) ^ (0xEDB88320u & -(crc & 1u));
    }
    return crc ^ 0xFFFFFFFFu;
}

// ── Helpers little-endian ─────────────────────────────────────────────────────
static void writeU32(uint8_t* p, uint32_t v) {
    p[0]=uint8_t(v); p[1]=uint8_t(v>>8); p[2]=uint8_t(v>>16); p[3]=uint8_t(v>>24);
}
static void writeI32(uint8_t* p, int32_t v)  { writeU32(p, uint32_t(v)); }
static void writeI64(uint8_t* p, int64_t v)  {
    writeU32(p,   uint32_t(uint64_t(v)));
    writeU32(p+4, uint32_t(uint64_t(v) >> 32));
}
static uint32_t readU32(const uint8_t* p) {
    return p[0]|(p[1]<<8)|(p[2]<<16)|(p[3]<<24);
}
static int32_t  readI32(const uint8_t* p) { return int32_t(readU32(p)); }
static int64_t  readI64(const uint8_t* p) {
    uint64_t lo = readU32(p), hi = readU32(p+4);
    return int64_t(lo | (hi << 32));
}

// ── Sérialisation ─────────────────────────────────────────────────────────────
bool AppData::serialize(uint8_t buf[SIZE]) const
{
    std::memset(buf, 0, SIZE);
    writeU32(buf +  0, MAGIC);
    writeU32(buf +  4, VERSION);
    writeI32(buf +  8, windowX);
    writeI32(buf + 12, windowY);
    writeI32(buf + 16, windowW);
    writeI32(buf + 20, windowH);
    writeI64(buf + 24, totalSecs);
    writeI64(buf + 32, launchCount);
    writeI64(buf + 40, lastLaunch);
    // [48..75] reserved
    writeU32(buf + 76, crc32(buf, 76));
    return true;
}

// ── Désérialisation ───────────────────────────────────────────────────────────
AppData AppData::deserialize(const uint8_t buf[SIZE])
{
    AppData d;
    d.windowX     = readI32(buf +  8);
    d.windowY     = readI32(buf + 12);
    d.windowW     = readI32(buf + 16);
    d.windowH     = readI32(buf + 20);
    d.totalSecs   = readI64(buf + 24);
    d.launchCount = readI64(buf + 32);
    d.lastLaunch  = readI64(buf + 40);
    return d;
}

// ── Validation ────────────────────────────────────────────────────────────────
bool AppData::validate(const uint8_t buf[SIZE])
{
    if (readU32(buf +  0) != MAGIC)   return false;
    if (readU32(buf +  4) != VERSION) return false;
    return readU32(buf + 76) == crc32(buf, 76);
}

// ── Chargement ────────────────────────────────────────────────────────────────
AppData AppData::load(const QString& path)
{
    QFile f(path);
    if (f.exists() && f.open(QIODevice::ReadOnly)) {
        QByteArray raw = f.readAll();
        f.close();
        if (raw.size() == SIZE) {
            const auto* buf = reinterpret_cast<const uint8_t*>(raw.constData());
            if (validate(buf))
                return deserialize(buf);
        }
    }
    return AppData{};
}

// ── Sauvegarde ────────────────────────────────────────────────────────────────
bool AppData::save(const QString& path) const
{
    uint8_t buf[SIZE];
    serialize(buf);
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return false;
    qint64 n = f.write(reinterpret_cast<const char*>(buf), SIZE);
    f.close();
    return n == SIZE;
}

// ── Formatage ─────────────────────────────────────────────────────────────────
QString AppData::formattedUsage() const
{
    int64_t t = totalSecs;
    int days  =  t / 86400;
    int hours = (t % 86400) / 3600;
    int mins  = (t % 3600)  / 60;
    int secs  =  t % 60;
    if (days > 0)
        return QString("%1d %2h %3m %4s")
            .arg(days).arg(hours)
            .arg(mins, 2, 10, QChar('0'))
            .arg(secs, 2, 10, QChar('0'));
    if (hours > 0)
        return QString("%1h %2m %3s")
            .arg(hours)
            .arg(mins, 2, 10, QChar('0'))
            .arg(secs, 2, 10, QChar('0'));
    return QString("%1m %2s")
        .arg(mins)
        .arg(secs, 2, 10, QChar('0'));
}
