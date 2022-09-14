#include "filterdialog.h"
#include "ui_filterdialog.h"

#include <QFileDialog>
#include <QFile>
#include <QMessageBox>

#include <QDebug>

#define HEX_LEN 88

FilterDialog::FilterDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::FilterDialog),
    m_donerow(0),
    m_temprow(0),
    m_curmode(0)
{    
    ui->setupUi(this);
    setWindowFlags(windowFlags() & ~(Qt::WindowCloseButtonHint));
    setWindowTitle(tr("过滤"));
    
    QStringList header;
    ui->filterTableWidget->setColumnCount(4);
    header << "低(HEX)" << "高(HEX)" << "备注" << " ";
    ui->filterTableWidget->setColumnWidth(0, HEX_LEN);
    ui->filterTableWidget->setColumnWidth(1, HEX_LEN);
    ui->filterTableWidget->setColumnWidth(2, HEX_LEN * 2);
    ui->filterTableWidget->setColumnWidth(3, 36);
    ui->filterTableWidget->setHorizontalHeaderLabels(header);

    QRegExp regHex8Exp("[0-9a-fA-FxX]{8}"); //创建了一个模式
    QRegExpValidator *hex8pattern = new QRegExpValidator(regHex8Exp, this); //创建了一个表达式
    ui->fromLowHexLineEdit->setValidator(hex8pattern);
    ui->toHighHexLineEdit->setValidator(hex8pattern);

    connect(ui->filterTableWidget, &QTableWidget::cellClicked, this, &FilterDialog::slotDelTableWidget);
    connect(ui->currentModeComboBox, SIGNAL(activated(int)), this, SLOT(slotModeComboBox(int)));
    connect(ui->additionButton, &QPushButton::clicked, this, &FilterDialog::slotAdditionButton);
    connect(ui->clearButton, &QPushButton::clicked, this, &FilterDialog::slotClearButton);
    connect(ui->exportButton, &QPushButton::clicked, this, &FilterDialog::slotExportButton);
    connect(ui->importButton, &QPushButton::clicked, this, &FilterDialog::slotImportButton);
    connect(ui->okButton, &QPushButton::clicked, this, &FilterDialog::slotOkButton);
    connect(ui->cancelButton, &QPushButton::clicked, this, &FilterDialog::slotCancelButton);

    slotModeComboBox(0);
}

FilterDialog::~FilterDialog()
{
    delete ui;
}

void FilterDialog::slotModeComboBox(int index)
{
    if (index > 0) {
        ui->fromLowHexLineEdit->setEnabled(true);
        ui->toHighHexLineEdit->setEnabled(true);
        ui->additionButton->setEnabled(true);
        ui->filterTableWidget->setEnabled(true);

        ui->clearButton->setEnabled(true);
        ui->exportButton->setEnabled(true);
        ui->importButton->setEnabled(true);
    } else {
        ui->fromLowHexLineEdit->setEnabled(false);
        ui->toHighHexLineEdit->setEnabled(false);
        ui->additionButton->setEnabled(false);
        ui->filterTableWidget->setEnabled(false);

        ui->clearButton->setEnabled(false);
        ui->exportButton->setEnabled(false);
        ui->importButton->setEnabled(false);
    }
}

void FilterDialog::slotAdditionButton()
{
    int endrow = ui->filterTableWidget->rowCount();
    QString lowHex = ui->fromLowHexLineEdit->text();
    QString highHex = ui->toHighHexLineEdit->text();

    if (ui->filterTableWidget->rowCount() >= 100) {
        QMessageBox::warning(this, tr("warning"), tr("The number of rows exceeds 100."));
        return ;
    }

    lowHex = tr("%1").arg(lowHex.toUInt(nullptr, 16), 8, 16, QChar('0'));
    highHex = tr("%1").arg(highHex.toUInt(nullptr, 16), 8, 16, QChar('0'));
    
    QTableWidgetItem *lowItem = new QTableWidgetItem(lowHex.toUpper());
    QTableWidgetItem *highItem = new QTableWidgetItem(highHex.toUpper());
    QTableWidgetItem *remarkItem = new QTableWidgetItem("");
    QTableWidgetItem *delItem = new QTableWidgetItem("[Del]");
    delItem->setTextColor(Qt::blue);

    m_temprow = endrow + 1;
    ui->filterTableWidget->setRowCount(m_temprow);
    ui->filterTableWidget->setItem(endrow, 0, lowItem);
    ui->filterTableWidget->setItem(endrow, 1, highItem);
    ui->filterTableWidget->setItem(endrow, 2, remarkItem);
    ui->filterTableWidget->setItem(endrow, 3, delItem);
    ui->filterTableWidget->setCurrentCell(endrow, 2);
}

void FilterDialog::slotClearButton()
{
    for (int i = ui->filterTableWidget->rowCount() - 1; i >= 0; --i) {
        ui->filterTableWidget->removeRow(i);
    }

    m_temprow = 0;
}

void FilterDialog::slotExportButton()
{
    QString filename = QFileDialog::getSaveFileName(this, tr("导出"), "./", "FILTER文件 (*.filter)");
    if (filename.isEmpty()) {
        return ;
    }

    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        QMessageBox::warning(this, tr("warning"), tr("cannot open file: %1").arg(filename));
        return ;
    }
    
    QTextStream out(&file);
    for (int row = 0; row < ui->filterTableWidget->rowCount(); ++row) {
        int columnCount = ui->filterTableWidget->columnCount();
        for (int col = 0; col < columnCount - 1; ++col) {
            out << ui->filterTableWidget->item(row, col)->text();
            if (col < (columnCount - 1)) {
                out << ",";
            }
        }
        out << "\n";
    }
    out.flush();
    file.close();

}

void FilterDialog::slotImportButton()
{
    QString filename = QFileDialog::getOpenFileName(this, tr("导入"), "./", "FILTER文件 (*.filter)");
    if (filename.isEmpty()) {
        return ;
    }

    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, tr("warning"), tr("cannot open file: %1").arg(filename));
        return ;
    }
    
    QByteArray readbyte = file.readAll();
    file.close();

    //qDebug() << "readbyte:" << readbyte;
    QString content = readbyte;
    if (content.size() > 6) {
        QStringList rowsOfData = content.split("\n");
        for (int row = 0; row < rowsOfData.size(); ++row) {
            if (rowsOfData.at(row).isEmpty()) {
                continue;
            }
            
            QStringList rowData = rowsOfData.at(row).split(",");
            m_temprow = row + 1;
            ui->filterTableWidget->setRowCount(m_temprow);
            for (int col = 0; col < ui->filterTableWidget->columnCount(); ++col) {
                QString cell = rowData[col];
                ui->filterTableWidget->setItem(row, col, new QTableWidgetItem(cell));
            }
            QTableWidgetItem *delItem = new QTableWidgetItem("[Del]");
            delItem->setTextColor(Qt::blue);
            ui->filterTableWidget->setItem(row, 3, delItem);
            ui->filterTableWidget->setCurrentCell(row, 2);
        }
    }
}

void FilterDialog::slotOkButton()
{
    m_donerow = m_temprow;
    m_curmode = ui->currentModeComboBox->currentIndex();
    accept();
}

void FilterDialog::slotCancelButton()
{
    for (int i = m_temprow - 1; i >= m_donerow; --i) {
        ui->filterTableWidget->removeRow(i);
        ui->filterTableWidget->setCurrentCell(i, 3);
    }
    m_temprow = m_donerow;
    ui->currentModeComboBox->setCurrentIndex(m_curmode);
    reject();
}

void FilterDialog::slotDelTableWidget(int row, int column)
{
    if (row >= 0 && 3 == column) {
        ui->filterTableWidget->removeRow(row);
        m_temprow--;
    }
}

int FilterDialog::getFilterParam(uint32_t *fromLowHex, uint32_t *toHighHex)
{
    int row = 0;

    //qDebug() << "m_donerow:" << m_donerow;
    for (row = 0; row < m_donerow; ++row) {
        QTableWidgetItem *lowItem = ui->filterTableWidget->item(row, 0);
        QString lowHex = lowItem->text();
        fromLowHex[row] = lowHex.toUInt(nullptr, 16);
        
        QTableWidgetItem *highItem = ui->filterTableWidget->item(row, 1);
        QString highHex = highItem->text();
        toHighHex[row] = highHex.toUInt(nullptr, 16);
        
        //qDebug("fromLowHex:0x%08x, toHighHex:0x%08x", fromLowHex[row], toHighHex[row]);
    }
    
    return row;
}

int FilterDialog::getFilterMode()
{
    return ui->currentModeComboBox->currentIndex();
}

