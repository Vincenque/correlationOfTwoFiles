#include <QApplication>
#include "crosscorrelationapp.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    CrossCorrelationApp window;
    window.show();
    return app.exec();
}
