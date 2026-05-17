#pragma once

// ── AppData — Fichier binaire persistant TileLauncher.dat ────────────────────
//
// Structure fixe de 80 octets avec magic number et checksum CRC32.
// Contient : position + taille fenêtre + compteur usage + numéro de lancement.
// Jamais vide : un fichier par défaut est généré si absent ou corrompu.
//
// Format sur disque (little-endian) :
//   [0..3]   magic      : 0x544C4441 ('TLDA')
//   [4..7]   version    : 0x00000002
//   [8..11]  windowX    : position X (int32)
//   [12..15] windowY    : position Y (int32)
//   [16..19] windowW    : largeur fenêtre (int32)
//   [20..23] windowH    : hauteur fenêtre (int32)
//   [24..31] totalSecs  : temps cumulé en secondes (int64)
//   [32..39] launchCount: nombre total de lancements (int64)
//   [40..47] lastLaunch : timestamp Unix dernier lancement (int64)
//   [48..75] reserved   : réservé (28 octets, zéros)
//   [76..79] crc32      : CRC32 des octets [0..75]

#include <QString>
#include <QPoint>
#include <QSize>
#include <cstdint>

struct AppData
{
    static constexpr uint32_t MAGIC   = 0x544C4441u; // 'TLDA'
    static constexpr uint32_t VERSION = 0x00000002u;
    static constexpr int      SIZE    = 80;

    // ── Données persistées ────────────────────────────────────────────────
    int32_t  windowX     = -1;       // -1 = pas encore positionné
    int32_t  windowY     = -1;
    int32_t  windowW     = -1;       // -1 = taille par défaut
    int32_t  windowH     = -1;
    int64_t  totalSecs   = 0;
    int64_t  launchCount = 0;
    int64_t  lastLaunch  = 0;

    // ── Accès haut niveau ─────────────────────────────────────────────────
    bool   hasPosition() const { return windowX >= 0 && windowY >= 0; }
    bool   hasSize()     const { return windowW > 0  && windowH > 0;  }
    QPoint position()    const { return {windowX, windowY}; }
    QSize  size()        const { return {windowW, windowH}; }
    void   setPosition(const QPoint& p) { windowX = p.x(); windowY = p.y(); }
    void   setSize(const QSize& s)      { windowW = s.width(); windowH = s.height(); }
    void   setGeometry(const QPoint& p, const QSize& s) { setPosition(p); setSize(s); }

    QString formattedUsage() const;

    // ── I/O ───────────────────────────────────────────────────────────────
    static AppData load(const QString& path);
    bool           save(const QString& path) const;

private:
    static uint32_t crc32(const uint8_t* data, int len);
    bool            serialize(uint8_t buf[SIZE]) const;
    static AppData  deserialize(const uint8_t buf[SIZE]);
    static bool     validate(const uint8_t buf[SIZE]);
};
