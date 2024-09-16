#ifndef CROSSCORRELATIONAPP_H
#define CROSSCORRELATIONAPP_H

#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QProgressBar>
#include <QButtonGroup>
#include <QRadioButton>

class CrossCorrelationApp : public QWidget {
    Q_OBJECT

public:
    CrossCorrelationApp(QWidget *parent = nullptr);
    ~CrossCorrelationApp();

private slots:
    void selectFile1();
    void selectFile2();
    void calculate();

private:
    QLineEdit *file1Edit;
    QPushButton *file1Button;
    QLineEdit *file2Edit;
    QPushButton *file2Button;
    QLineEdit *startSampleEdit;
    QLineEdit *numSamplesEdit;
    QPushButton *calculateButton;

    QButtonGroup *fileSelector;
};

#endif // CROSSCORRELATIONAPP_H

