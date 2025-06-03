#include <QThread>
#include <QDebug>
#include "libxr.hpp"
#include "TerminalBackend.hpp"
#include "DeviceManager.hpp"

class AppMain : public QThread
{
    Q_OBJECT

public:
protected:
    void run() override
    {
        // 这里是线程执行的代码
        // 可以添加其他耗时任务
        for (int i = 0; i < 5; ++i)
        {
            XR_LOG_PASS("Hello, World!");
            QThread::msleep(1000); // 每秒输出一次
        }
    }
};
