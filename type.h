#ifndef TYPE_H
#define TYPE_H

#include <QString>

struct NetSpeedInfo {
    unsigned long upBytes;
    unsigned long downBytes;
    QString upSpeed;
    QString downSpeed;
};

#endif
