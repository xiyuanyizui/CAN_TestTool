#ifndef TRIGGERDIALOG_H
#define TRIGGERDIALOG_H

#include "ControlCAN.h"
#include "sendthread.h"

#include <QDialog>
#include <QAbstractButton>
#include <QTableWidget>

#define TRIGGERSET_PATH "./USER_FILE/triggerset.csv"

namespace Ui {
class TriggerDialog;
}

class TriggerDialog : public QDialog
{
    Q_OBJECT

public:
    explicit TriggerDialog(QWidget *parent = 0);
    ~TriggerDialog();
    int getTriggerMode();
    void getTriggerParam(QMap<QString, QList<SEND_DATA_T>> &trigmap);
    void exportToFileCSV(const QString &filepath, const QTableWidget *tableWidget);
    void importFromFileCSV(const QString &filepath, QTableWidget *tableWidget);

public slots:
    void slotCurrentMode(int index);
    void slotBoxButton(QAbstractButton *button);
    void slotExportButton();
    void slotImportButton();
    void slotAdditionButton();
    void slotUpdateButton();
    void slotDeleteButton();
    void slotCondDataTextChanged(const QString &text);
    void slotTrigDataTextChanged(const QString &text);
    void slotCellDoubleClicked(int row);

private:
    Ui::TriggerDialog *ui;
    int m_donerow;
    int m_temprow;
    int m_curmode;
};

#endif // TRIGGERDIALOG_H
