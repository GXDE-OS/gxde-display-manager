#include <QApplication>
#include <QPointer>
#include <QScreen>
#include <QTest>
#include <QWidget>

#include "multiscreenmanager.h"

class MultiScreenManagerTest : public QObject
{
    Q_OBJECT

private slots:
    void recreatesEveryManagedFrame();
};

void MultiScreenManagerTest::recreatesEveryManagedFrame()
{
    int created = 0;
    int destroyed = 0;
    QList<QPointer<QWidget>> originalFrames;
    MultiScreenManager manager;

    manager.register_for_mutil_screen([&](QScreen *) {
        QWidget *frame = new QWidget;
        ++created;
        connect(frame, &QObject::destroyed, this, [&destroyed] {
            ++destroyed;
        });
        originalFrames.append(frame);
        return frame;
    });

    const int screenCount = qApp->screens().size();
    QCOMPARE(created, screenCount);

    for (int cycle = 1; cycle <= 3; ++cycle) {
        manager.recreateFrames();

        QCOMPARE(destroyed, screenCount * cycle);
        QCOMPARE(created, screenCount * (cycle + 1));
        for (int i = 0; i < screenCount * cycle; ++i) {
            QVERIFY(originalFrames.at(i).isNull());
        }
    }
}

QTEST_MAIN(MultiScreenManagerTest)
#include "MultiScreenManagerTest.moc"
