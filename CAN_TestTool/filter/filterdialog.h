#ifndef FILTERDIALOG_H
#define FILTERDIALOG_H

#include <QDialog>
#include <QAbstractButton>

namespace Ui {
class FilterDialog;
}

class FilterDialog : public QDialog
{
    Q_OBJECT

public:
    explicit FilterDialog(QWidget *parent = 0);
    ~FilterDialog();
    int getFilterMode();
    int getFilterParam(uint32_t *fromLowHex, uint32_t *toHighHex);

public slots:
    void slotModeComboBox(int index);
    void slotAdditionButton();
    void slotClearButton();
    void slotExportButton();
    void slotImportButton();
    void slotOkButton();
    void slotCancelButton();
    void slotDelTableWidget(int row, int column);

private:
    Ui::FilterDialog *ui;
    int m_donerow;
    int m_temprow;
    int m_curmode;
};

#endif // FILTERDIALOG_H
