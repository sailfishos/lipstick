#include <QObject>

namespace NemoDeviceLock
{

class DeviceLock : public QObject
{
    Q_OBJECT
public:
    enum LockState
    {
        Unlocked = 0,
        Locked,
        ManagerLockout,
        TemporaryLockout,
        PermanentLockout,
        Undefined
    };
    Q_ENUM(LockState)

    DeviceLock(QObject *parent = nullptr)
        : QObject(parent)
        , m_state(Unlocked)
        , m_showNotifications(true)
    {
    }

    LockState state() const { return m_state; }
    void setState(LockState state) { m_state = state; }

    bool showNotifications() const { return m_showNotifications; }
    void setShowNotifications(bool show) { m_showNotifications = show; }

private:
    LockState m_state;
    bool m_showNotifications;
};

}
