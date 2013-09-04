#ifndef KEYFRAMEHOLDER_H
#define KEYFRAMEHOLDER_H
#include <QVector>
#include <QTime>
/*
struct KeyFrame
{

};
*/
class KeyFrameHolder
{
public:
    static QVector<QTime> m_Pauses;
    static void reorder() {
        qSort(m_Pauses);
    }
};

#endif // KEYFRAMEHOLDER_H
