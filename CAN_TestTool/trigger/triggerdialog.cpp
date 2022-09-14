#include "triggerdialog.h"
#include "ui_triggerdialog.h"

#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QMessageBox>

#include <QDebug>

#define HEX_LEN 88

TriggerDialog::TriggerDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::TriggerDialog),
    m_donerow(0),
    m_temprow(0),
    m_curmode(0)
{    
    ui->setupUi(this);
    setWindowFlags(windowFlags() & ~(Qt::WindowCloseButtonHint));
    setWindowTitle(tr("触发设置"));

    QStringList header;
    ui->tableWidget->setColumnCount(13);
    header << "条件-帧ID(HEX)" << "DLC(HEX)" << "帧类型" << "帧格式" << "数据(HEX)"
           << "触发-帧ID(HEX)" << "DLC(HEX)" << "帧类型" << "帧格式" << "数据(HEX)"
           << "发送格式" << "发送次数" << "时间间隔(MS)";
    ui->tableWidget->setHorizontalHeaderLabels(header);
    ui->tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);

    ui->condDLCLineEdit->setEnabled(false);
    ui->trigDLCLineEdit->setEnabled(false);

    QRegExp regDecExp("[0-9]{10}"); //创建了一个模式
    QRegExp regHex8Exp("[0-9a-fA-FxX]{8}");
    QRegExp regHex512Exp("[0-9a-fA-FxX ]{512}");
    QRegExpValidator *decpattern = new QRegExpValidator(regDecExp, this); //创建了一个表达式
    QRegExpValidator *hex8pattern = new QRegExpValidator(regHex8Exp, this);
    QRegExpValidator *hex512pattern = new QRegExpValidator(regHex512Exp, this);
    ui->condFrameIDLineEdit->setValidator(hex8pattern);
    ui->condDLCLineEdit->setValidator(hex8pattern);
    ui->condDataLineEdit->setValidator(hex512pattern);
    
    ui->trigFrameIDLineEdit->setValidator(hex8pattern);
    ui->trigDLCLineEdit->setValidator(hex8pattern);
    ui->trigDataLineEdit->setValidator(hex512pattern);
    ui->trigSendNumLineEdit->setValidator(decpattern);
    ui->trigTimeIntervalLineEdit->setValidator(decpattern);

    ui->condGroupBox->setEnabled(false);
    ui->trigGroupBox->setEnabled(false);

    QDir dir;
    dir.mkdir("./USER_FILE");
    QFile file(TRIGGERSET_PATH);
    if (!file.exists()) {
        file.open(QIODevice::ReadWrite);
        file.close();
    }
    importFromFileCSV(TRIGGERSET_PATH, ui->tableWidget);

    connect(ui->currentModeComboBox, SIGNAL(activated(int)), this, SLOT(slotCurrentMode(int)));

    connect(ui->tableWidget, &QTableWidget::cellDoubleClicked, this, &TriggerDialog::slotCellDoubleClicked);
    connect(ui->condDataLineEdit, &QLineEdit::textChanged, this, &TriggerDialog::slotCondDataTextChanged);
    connect(ui->trigDataLineEdit, &QLineEdit::textChanged, this, &TriggerDialog::slotTrigDataTextChanged);

    connect(ui->exportButton, &QPushButton::clicked, this, &TriggerDialog::slotExportButton);
    connect(ui->importButton, &QPushButton::clicked, this, &TriggerDialog::slotImportButton);

    connect(ui->additionButton, &QPushButton::clicked, this, &TriggerDialog::slotAdditionButton);
    connect(ui->updateButton, &QPushButton::clicked, this, &TriggerDialog::slotUpdateButton);
    connect(ui->deleteButton, &QPushButton::clicked, this, &TriggerDialog::slotDeleteButton);
    connect(ui->buttonBox, &QDialogButtonBox::clicked, this, &TriggerDialog::slotBoxButton);
}

TriggerDialog::~TriggerDialog()
{
    delete ui;
}

void TriggerDialog::exportToFileCSV(const QString &filepath, const QTableWidget *tableWidget)
{
    QFile file(filepath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        QMessageBox::warning(this, tr("warning"), tr("cannot open file: %1").arg(filepath));
        return ;
    }
    
    QTextStream out(&file);
    for (int row = 0; row < tableWidget->rowCount(); ++row) {
        int columnCount = tableWidget->columnCount();
        for (int col = 0; col < columnCount; ++col) {
            out << tableWidget->item(row, col)->text();
            if (col < (columnCount - 1)) {
                out << ",";
            }
        }
        out << "\n";
    }
    out.flush();
    file.close();
}

void TriggerDialog::importFromFileCSV(const QString &filepath, QTableWidget *tableWidget)
{
    QFile file(filepath);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, tr("warning"), tr("cannot open file: %1").arg(filepath));
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
            tableWidget->setRowCount(m_temprow);
            for (int col = 0; col < tableWidget->columnCount(); ++col) {
                QString cell = rowData[col];
                tableWidget->setItem(row, col, new QTableWidgetItem(cell.toUpper()));
                tableWidget->setCurrentCell(row, col);
            }
        }
    }
}

void TriggerDialog::slotCurrentMode(int index)
{
    if (index > 0) {
        ui->condGroupBox->setEnabled(true);
        ui->trigGroupBox->setEnabled(true);
    } else {
        ui->condGroupBox->setEnabled(false);
        ui->trigGroupBox->setEnabled(false);
    }
}

void TriggerDialog::slotBoxButton(QAbstractButton *button)
{
    if (tr("OK") == button->text()) {
        m_donerow = m_temprow;
        m_curmode = ui->currentModeComboBox->currentIndex();
        exportToFileCSV(TRIGGERSET_PATH, ui->tableWidget);
    } else if (tr("Cancel") == button->text()) {
        for (int i = m_temprow - 1; i >= m_donerow; --i) {
            ui->tableWidget->removeRow(i);
            ui->tableWidget->setCurrentCell(i, 13);
        }
        m_temprow = m_donerow;
        ui->currentModeComboBox->setCurrentIndex(m_curmode);
    }
}

void TriggerDialog::slotExportButton()
{
    QString filename = QFileDialog::getSaveFileName(this, tr("导出"),
        "C:/Users/Administrator/Desktop/", "CSV文件 (*.csv)");
    if (!filename.isEmpty()) {
        exportToFileCSV(filename, ui->tableWidget);
    }
}

void TriggerDialog::slotImportButton()
{
    QString filename = QFileDialog::getOpenFileName(this, tr("导入"),
        "C:/Users/Administrator/Desktop/", "CSV文件 (*.csv)");
    if (!filename.isEmpty()) {
        importFromFileCSV(filename, ui->tableWidget);
    }
}

void TriggerDialog::slotAdditionButton()
{
    int endrow = ui->tableWidget->rowCount();
    QString condFrameID = ui->condFrameIDLineEdit->text();
    QString condDLC     = ui->condDLCLineEdit->text();
    int condFrameType = ui->condFrameTypeComboBox->currentIndex();
    int condFrameFormat = ui->condFrameFormatComboBox->currentIndex();
    QString condData = ui->condDataLineEdit->text();
    
    QString trigFrameID = ui->trigFrameIDLineEdit->text();
    QString trigDLC = ui->trigDLCLineEdit->text();
    int trigFrameType = ui->trigFrameTypeComboBox->currentIndex();
    int trigFrameFormat = ui->trigFrameFormatComboBox->currentIndex();
    QString trigData = ui->trigDataLineEdit->text();
    int trigSendFormat = ui->trigSendFormatComboBox->currentIndex();
    QString trigSendNum = ui->trigSendNumLineEdit->text();
    QString trigTimeInterval = ui->trigTimeIntervalLineEdit->text();

    if (ui->tableWidget->rowCount() >= 100) {
        QMessageBox::warning(this, tr("warning"), tr("The number of rows exceeds 100."));
        return ;
    }

    if (trigFrameID.isEmpty() || trigData.isEmpty()) {
        QMessageBox::warning(this, tr("warning"), tr("Trigger frame id or data is null."));
        return ;
    }

    condFrameID = tr("%1").arg(condFrameID.toUInt(nullptr, 16), 8, 16, QChar('0'));
    trigFrameID = tr("%1").arg(trigFrameID.toUInt(nullptr, 16), 8, 16, QChar('0'));
    
    int col  = 0;
    m_temprow = endrow + 1;
    ui->tableWidget->setRowCount(m_temprow);
    ui->tableWidget->setItem(endrow, col++, new QTableWidgetItem(condFrameID.toUpper()));
    ui->tableWidget->setItem(endrow, col++, new QTableWidgetItem(condDLC.toUpper()));
    ui->tableWidget->setItem(endrow, col++, new QTableWidgetItem(QString::number(condFrameType)));
    ui->tableWidget->setItem(endrow, col++, new QTableWidgetItem(QString::number(condFrameFormat)));
    ui->tableWidget->setItem(endrow, col++, new QTableWidgetItem(condData.toUpper()));
    
    ui->tableWidget->setItem(endrow, col++, new QTableWidgetItem(trigFrameID.toUpper()));
    ui->tableWidget->setItem(endrow, col++, new QTableWidgetItem(trigDLC.toUpper()));
    ui->tableWidget->setItem(endrow, col++, new QTableWidgetItem(QString::number(trigFrameType)));
    ui->tableWidget->setItem(endrow, col++, new QTableWidgetItem(QString::number(trigFrameFormat)));
    ui->tableWidget->setItem(endrow, col++, new QTableWidgetItem(trigData.toUpper()));
    ui->tableWidget->setItem(endrow, col++, new QTableWidgetItem(QString::number(trigSendFormat)));
    ui->tableWidget->setItem(endrow, col++, new QTableWidgetItem(trigSendNum));
    ui->tableWidget->setItem(endrow, col++, new QTableWidgetItem(trigTimeInterval));
    
    ui->tableWidget->setCurrentCell(endrow, col - 1);
}

void TriggerDialog::slotUpdateButton()
{
    int row = ui->tableWidget->currentRow();
    if (-1 == row) {
        return ;
    }

    QString condFrameID = ui->condFrameIDLineEdit->text();
    QString condDLC     = ui->condDLCLineEdit->text();
    int condFrameType = ui->condFrameTypeComboBox->currentIndex();
    int condFrameFormat = ui->condFrameFormatComboBox->currentIndex();
    QString condData = ui->condDataLineEdit->text();
    
    QString trigFrameID = ui->trigFrameIDLineEdit->text();
    QString trigDLC = ui->trigDLCLineEdit->text();
    int trigFrameType = ui->trigFrameTypeComboBox->currentIndex();
    int trigFrameFormat = ui->trigFrameFormatComboBox->currentIndex();
    QString trigData = ui->trigDataLineEdit->text();
    int trigSendFormat = ui->trigSendFormatComboBox->currentIndex();
    QString trigSendNum = ui->trigSendNumLineEdit->text();
    QString trigTimeInterval = ui->trigTimeIntervalLineEdit->text();

    int col = 0;
    ui->tableWidget->item(row, col++)->setText(condFrameID.toUpper());
    ui->tableWidget->item(row, col++)->setText(condDLC.toUpper());
    ui->tableWidget->item(row, col++)->setText(QString::number(condFrameType));
    ui->tableWidget->item(row, col++)->setText(QString::number(condFrameFormat));
    ui->tableWidget->item(row, col++)->setText(condData.toUpper());
    ui->tableWidget->item(row, col++)->setText(trigFrameID.toUpper());
    ui->tableWidget->item(row, col++)->setText(trigDLC.toUpper());
    ui->tableWidget->item(row, col++)->setText(QString::number(trigFrameType));
    ui->tableWidget->item(row, col++)->setText(QString::number(trigFrameFormat));
    ui->tableWidget->item(row, col++)->setText(trigData.toUpper());
    ui->tableWidget->item(row, col++)->setText(QString::number(trigSendFormat));
    ui->tableWidget->item(row, col++)->setText(trigSendNum);
    ui->tableWidget->item(row, col++)->setText(trigTimeInterval);
}

void TriggerDialog::slotDeleteButton()
{
    int row = ui->tableWidget->currentRow();
    if (-1 == row) {
        return ;
    }
    
    ui->tableWidget->removeRow(row);
    m_temprow--;
}

void TriggerDialog::slotCondDataTextChanged(const QString &text)
{
    QString hexstr = text;

    hexstr.replace(QLatin1String(" "), QLatin1String(""));
    int bytes = hexstr.length() / 2;
    ui->condDLCLineEdit->setText(QString("%1").arg(bytes, 2, 16, QLatin1Char('0')));
}

void TriggerDialog::slotTrigDataTextChanged(const QString &text)
{
    QString hexstr = text;

    hexstr.replace(QLatin1String(" "), QLatin1String(""));
    int bytes = hexstr.length() / 2;
    ui->trigDLCLineEdit->setText(QString("%1").arg(bytes, 2, 16, QLatin1Char('0')));
}

void TriggerDialog::slotCellDoubleClicked(int row)
{
    int col = 0;
    QString condFrameID = ui->tableWidget->item(row, col++)->text();
    QString condDLC = ui->tableWidget->item(row, col++)->text();
    int condFrameType = ui->tableWidget->item(row, col++)->text().toInt();
    int condFrameFormat = ui->tableWidget->item(row, col++)->text().toInt();
    QString condData = ui->tableWidget->item(row, col++)->text();
    
    QString trigFrameID = ui->tableWidget->item(row, col++)->text();
    QString trigDLC = ui->tableWidget->item(row, col++)->text();
    int trigFrameType = ui->tableWidget->item(row, col++)->text().toInt();
    int trigFrameFormat = ui->tableWidget->item(row, col++)->text().toInt();
    QString trigData = ui->tableWidget->item(row, col++)->text();
    int trigSendFormat = ui->tableWidget->item(row, col++)->text().toInt();
    QString trigSendNum = ui->tableWidget->item(row, col++)->text();
    QString trigTimeInterval = ui->tableWidget->item(row, col++)->text();
    
    ui->condFrameIDLineEdit->setText(condFrameID);
    ui->condDLCLineEdit->setText(condDLC);
    ui->condFrameTypeComboBox->setCurrentIndex(condFrameType);
    ui->condFrameFormatComboBox->setCurrentIndex(condFrameFormat);
    ui->condDataLineEdit->setText(condData);

    ui->trigFrameIDLineEdit->setText(trigFrameID);
    ui->trigDLCLineEdit->setText(trigDLC);
    ui->trigFrameTypeComboBox->setCurrentIndex(trigFrameType);
    ui->trigFrameFormatComboBox->setCurrentIndex(trigFrameFormat);
    ui->trigDataLineEdit->setText(trigData);
    ui->trigSendFormatComboBox->setCurrentIndex(trigSendFormat);
    ui->trigSendNumLineEdit->setText(trigSendNum);
    ui->trigTimeIntervalLineEdit->setText(trigTimeInterval);
}

int TriggerDialog::getTriggerMode()
{
    return ui->currentModeComboBox->currentIndex();
}

void TriggerDialog::getTriggerParam(QMap<QString, QList<SEND_DATA_T>> &trigmap)
{
    int count = 0;
    bool ok = false;

    trigmap.clear();

    for (int row = 0; row < m_donerow; ++row) {
        int col = 0;
        QString condFrameID = ui->tableWidget->item(row, col++)->text();
        QString condDLC = ui->tableWidget->item(row, col++)->text();
        QString condFrameType = ui->tableWidget->item(row, col++)->text();
        QString condFrameFormat = ui->tableWidget->item(row, col++)->text();
        QString condData = ui->tableWidget->item(row, col++)->text();

        QString key;
        if (!condFrameID.isEmpty()) {
            key.append(condFrameID);
            key.append(":");
        }
        key.append(condDLC);
        key.append(":");
        key.append(condFrameType);
        key.append(":");
        key.append(condFrameFormat);
        if (!condData.isEmpty()) {
            condData.replace(QLatin1String(" "), QLatin1String(""));
            key.append(":");
            key.append(condData);
        }

        QList<SEND_DATA_T> value;
        if (trigmap.contains(key)) {
            QMap<QString, QList<SEND_DATA_T>>::iterator it = trigmap.find(key);
            value = it.value();
        }

        QString trigFrameID = ui->tableWidget->item(row, col++)->text();
        QString trigDLC = ui->tableWidget->item(row, col++)->text();
        int trigFrameType = ui->tableWidget->item(row, col++)->text().toInt();
        int trigFrameFormat = ui->tableWidget->item(row, col++)->text().toInt();
        QString trigData = ui->tableWidget->item(row, col++)->text();
        int trigSendFormat = ui->tableWidget->item(row, col++)->text().toInt();
        QString trigSendNum = ui->tableWidget->item(row, col++)->text();
        QString trigTimeInterval = ui->tableWidget->item(row, col++)->text();

        SEND_DATA_T send;
        memset(&send, 0, sizeof(SEND_DATA_T));
        send.can.ID         = trigFrameID.toUInt(&ok, 16);
        send.can.ExternFlag = trigFrameType;
        send.can.RemoteFlag = trigFrameFormat;
        send.can.SendType   = trigSendFormat;
        send.can.DataLen    = trigDLC.toInt(&ok, 16);
        trigData.replace(QLatin1String(" "), QLatin1String(""));

        int trigdlc = trigDLC.toInt(&ok, 16);
        if (trigdlc <= 8) {
            int realconut = trigdlc;
            for (int i = 0, count = 0; ; i += 2) {
                if (realconut-- > 0) {
                    QString byte = trigData.mid(i, 2);
                    send.can.Data[count++] = byte.toInt(&ok, 16);
                } else {
                    send.can.Data[count++] = 0xff;
                }
                if (8 == count) {
                    break;
                }
            }
            send.sendnum = trigSendNum.toInt();
            send.intervalms = trigTimeInterval.toInt();
            value << send;
        } else {
            BYTE frameid = 0x21;
            bool isoneframe = true;
            count = 0;
            send.can.DataLen = 8;
            send.can.Data[count++] =  0x10;
            int realconut = trigdlc;
            for (int i = 0; ; i += 2) {
                if (realconut-- > 0) {
                    QString byte = trigData.mid(i, 2);
                    send.can.Data[count++] = byte.toInt(&ok, 16);
                } else {
                    send.can.Data[count++] = 0xff;
                }

                if (8 == count) {
                    if (isoneframe) {
                        isoneframe = false;
                        send.sendnum = trigSendNum.toInt();
                        send.intervalms = trigTimeInterval.toInt();
                    } else {
                        send.sendnum = 1;
                        send.intervalms = 10;
                    }
                    value << send;
                    if (realconut <= 0) {
                        break;
                    }
                    count = 0;
                    send.can.Data[count++] = frameid;
                    if (++frameid > 0x2f) {
                        frameid = 0x20;
                    }
                }
            }
        }
        qDebug() << "[" << row << "Trigger] key:" << key << "trigFrameID:" 
                 << trigFrameID << "trigData:" << trigData;
        trigmap.insert(key, value);
    }
}

