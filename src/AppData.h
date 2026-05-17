#pragma once

// ── AppData — Fichier binaire persistant TileLauncher.dat ────────────────────
//
// Structure fixe de 64 octets avec magic number et checksum CRC32.
// Contient : position fenêtre + compteur usage + numéro de lancement.
// Jamais vide : un fichier par défaut est généré si absent ou corrompu.
//
// Format sur disque (little-endian) :
//   [0..3]   magic      : 0x544C4441 ('TLDA')
//   [4..7]   version    : 0x00000001
//   [8..11]  windowX    : position X (int32)
//   [12..15] windowY    : position Y (int32)
//   [16..23] totalSecs  : temps cumulé en secondes (int64)
//   [24..31] launchCount: nombre total de lancements (int64)
//   [32..39] lastLaunch : timestamp Unix dernier lancement (int64)
//   [40..59] reserved   : réservé (20 octets, zéros)
//   [60..63] crc32      : CRC32 des octets [0..59]

#include <QString>
#include <QPoint>
#include <cstdint>

struct AppData
{
    static constexpr uint32_t MAGIC   = 0x544C4441u; // 'TLDA'
    static constexpr uint32_t VERSION = 0x00000001u;
    static constexpr int      SIZE    = 64;

    // ── Données persistées ────────────────────────────────────────────────
    int32_t  windowX     = -1;       // -1 = pas encore positionné
    int32_t  windowY     = -1;
    int64_t  totalSecs   = 0;        // Temps cumulé toutes sessions (secondes)
    int64_t  launchCount = 0;        // Nombre de lancements
    int64_t  lastLaunch  = 0;        // Timestamp Unix du dernier lancement

    // ── Accès haut niveau ─────────────────────────────────────────────────
    bool     hasPosition() const { return windowX >= 0 && windowY >= 0; }
    QPoint   position()   const { return {windowX, windowY}; }
    void     setPosition(const QPoint& p) { windowX = p.x(); windowY = p.y(); }

    QString  formattedUsage() const;   // "12h 34m 56s"

    // ── I/O ───────────────────────────────────────────────────────────────
    static AppData load(const QString& path);  // Charge ou crée par défaut
    bool           save(const QString& path) const;

private:
    static uint32_t crc32(const uint8_t* data, int len);
    bool            serialize(uint8_t buf[SIZE]) const;
    static AppData  deserialize(const uint8_t buf[SIZE]);
    static bool     validate(const uint8_t buf[SIZE]);
};
