/*===================================================================
APM_PLANNER Open Source Ground Control Station

(c) 2013 APM_PLANNER PROJECT <http://www.diydrones.com>

This file is part of the APM_PLANNER project

    APM_PLANNER is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    APM_PLANNER is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with APM_PLANNER. If not, see <http://www.gnu.org/licenses/>.

======================================================================*/

#include "QsLog.h"
#include "SerialSettingsDialog.h"
#include "Radio3DRConfig.h"
#include "Radio3DRSettings.h"

#include "configuration.h"

#include <QSettings>
#include <qserialportinfo.h>
#include <qserialport.h>
#include <QTimer>
#include <QMessageBox>

Radio3DRConfig::Radio3DRConfig(QWidget *parent) : QWidget(parent),
    m_radioSettings(NULL)
{
    ui.setupUi(this);
    m_settingsDialog = new SettingsDialog;

    ui.settingsButton->setEnabled(true);

    addBaudComboBoxConfig(ui.baudPortComboBox);
    fillPortsInfo(*ui.linkPortComboBox);

    addRadioBaudComboBoxConfig(*ui.baudComboBox);
    addRadioBaudComboBoxConfig(*ui.baudComboBox_remote);
    addRadioAirBaudComboBoxConfig(*ui.airBaudComboBox);
    addRadioAirBaudComboBoxConfig(*ui.airBaudComboBox_remote);
    addTxPowerComboBoxConfig(*ui.txPowerComboBox);
    addTxPowerComboBoxConfig(*ui.txPowerComboBox_remote);

    loadSavedSerialSettings();

    initConnections();

    //Keep refreshing the serial port list
    m_timer = new QTimer(this);
    connect(m_timer,SIGNAL(timeout()),this,SLOT(populateSerialPorts()));
}

Radio3DRConfig::~Radio3DRConfig()
{
}

void Radio3DRConfig::addBaudComboBoxConfig(QComboBox *comboBox)
{
    comboBox->addItem(QLatin1String("115200"), QSerialPort::Baud115200);
    comboBox->addItem(QLatin1String("57600"), QSerialPort::Baud57600);
    comboBox->addItem(QLatin1String("38400"), QSerialPort::Baud38400);
    comboBox->addItem(QLatin1String("19200"), QSerialPort::Baud19200);
    comboBox->addItem(QLatin1String("9600"), QSerialPort::Baud9600);
    comboBox->addItem(QLatin1String("4800"), QSerialPort::Baud4800);
    comboBox->addItem(QLatin1String("2400"), QSerialPort::Baud2400);
    comboBox->addItem(QLatin1String("1200"), QSerialPort::Baud1200);

    // Set combobox to default baud rate of 57600.
    comboBox->setCurrentIndex(2);
}

void Radio3DRConfig::fillPortsInfo(QComboBox &comboxBox)
{
    QLOG_TRACE() << "3DR Radio fillPortsInfo ";
    foreach (const QSerialPortInfo &info, QSerialPortInfo::availablePorts()) {
        QStringList list;
        list << info.portName()
             << info.description()
             << info.manufacturer()
             << info.systemLocation()
             << (info.vendorIdentifier() ? QString::number(info.vendorIdentifier(), 16) : QString())
             << (info.productIdentifier() ? QString::number(info.productIdentifier(), 16) : QString());

        int found = comboxBox.findData(list);
        if ((found == -1)&& info.manufacturer().contains("FTDI")) {
            QLOG_INFO() << "Inserting " << list.first();
            comboxBox.insertItem(0,list[0], list);
        } else {
            // Do nothing as the port is already listed
        }
    }}

void Radio3DRConfig::loadSavedSerialSettings()
{
    // Load defaults from settings
    QSettings settings(QGC::COMPANYNAME, QGC::APPNAME);
    settings.sync();
    if (settings.contains("3DRRADIO_COMM_PORT"))
    {
        m_settings.name = settings.value("3DRRADIO_COMM_PORT").toString();
        m_settings.baudRate = settings.value("3DRRADIO_COMM_BAUD").toInt();
        m_settings.parity = static_cast<QSerialPort::Parity>
                (settings.value("3DRRADIO_COMM_PARITY").toInt());
        m_settings.stopBits = static_cast<QSerialPort::StopBits>
                (settings.value("3DRRADIO_COMM_STOPBITS").toInt());
        m_settings.dataBits = static_cast<QSerialPort::DataBits>
                (settings.value("3DRRADIO_COMM_DATABITS").toInt());
        m_settings.flowControl = static_cast<QSerialPort::FlowControl>
                (settings.value("3DRRADIO_COMM_FLOW_CONTROL").toInt());

        ui.linkPortComboBox->setCurrentIndex(ui.linkPortComboBox->findText(m_settings.name));
        ui.baudPortComboBox->setCurrentIndex(ui.baudPortComboBox->findData(m_settings.baudRate));
    } else {
        // init the structure
    }
}

void Radio3DRConfig::saveSerialSettings()
{
    // Store settings
    QSettings settings(QGC::COMPANYNAME, QGC::APPNAME);
    settings.setValue("3DRRADIO_COMM_PORT", m_settings.name);
    settings.setValue("3DRRADIO_COMM_BAUD", m_settings.baudRate);
    settings.setValue("3DRRADIO_COMM_PARITY", m_settings.parity);
    settings.setValue("3DRRADIO_COMM_STOPBITS", m_settings.stopBits);
    settings.setValue("3DRRADIO_COMM_DATABITS", m_settings.dataBits);
    settings.setValue("3DRRADIO_COMM_FLOW_CONTROL", m_settings.flowControl);
    settings.sync();
}

void Radio3DRConfig::showEvent(QShowEvent *event)
{
    Q_UNUSED(event);
    // Start refresh Timer
    QLOG_DEBUG() << "3DR Radio Start Serial Port Scanning";
    m_timer->start(2000);
    loadSavedSerialSettings();
}

void Radio3DRConfig::hideEvent(QHideEvent *event)
{
    Q_UNUSED(event);
    // Stop the port list refeshing
    QLOG_DEBUG() << "3DR Radio Stop Serial Port Scanning";
    m_timer->stop();
    saveSerialSettings();
    QLOG_DEBUG() << "3DR Radio Remove Conenction to Serial Port";
    delete m_radioSettings;
}

void Radio3DRConfig::populateSerialPorts()
{
    QLOG_TRACE() << "populateSerialPorts";
    fillPortsInfo(*ui.linkPortComboBox);
}

void Radio3DRConfig::initConnections()
{
    // Ui Connections
    connect(ui.loadSettingsButton, SIGNAL(clicked()), this, SLOT(readRadioSettings()));

    connect(ui.saveLocalSettingsButton, SIGNAL(clicked()), this, SLOT(writeLocalRadioSettings()));
    connect(ui.saveRemoteSettingsButton, SIGNAL(clicked()), this, SLOT(writeRemoteRadioSettings()));
    connect(ui.resetDefaultsButton, SIGNAL(clicked()), this, SLOT(resetRadioSettingsToDefaults()));
    connect(ui.copyToRemoteButton, SIGNAL(clicked()), this, SLOT(copyLocalSettingsToRemote()));

    connect(ui.settingsButton, SIGNAL(released()), m_settingsDialog, SLOT(show()));

    connect(ui.baudPortComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(setBaudRate(int)));
    connect(ui.linkPortComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(setLink(int)));
}

void Radio3DRConfig::serialPortOpenFailure(int error, QString errorString)
{
    Q_UNUSED(error);
    QLOG_ERROR() << "Serial Port Open Crtical Error!" << errorString;
    QMessageBox::critical(this, tr("Cannot Open Serial Port"), errorString);
}

void Radio3DRConfig::setBaudRate(int index)
{
    m_settings.baudRate = static_cast<QSerialPort::BaudRate>(
                ui.baudPortComboBox->itemData(index).toInt());
    QLOG_INFO() << "Changed Baud to:" << m_settings.baudRate;

}

void Radio3DRConfig::setLink(int index)
{
    if (index == -1)
    {
        return;
    }
    m_settings.name = ui.linkPortComboBox->itemData(index).toStringList()[0];
    QLOG_INFO() << "Changed Link to:" << m_settings.name;

}

void Radio3DRConfig::readRadioSettings()
{
    QLOG_INFO() << "read 3DR Radio Settings";
    if (m_radioSettings == NULL) {
        m_radioSettings = new Radio3DRSettings(this);
        connect(m_radioSettings, SIGNAL(localReadComplete(Radio3DREeprom&, bool)),
                this, SLOT(localReadComplete(Radio3DREeprom&, bool)));
        connect(m_radioSettings, SIGNAL(remoteReadComplete(Radio3DREeprom&,bool)),
                this, SLOT(remoteReadComplete(Radio3DREeprom&, bool)));
        connect(m_radioSettings, SIGNAL(serialPortOpenFailure(int,QString)),
                this, SLOT(serialPortOpenFailure(int, QString)));
        connect(m_radioSettings, SIGNAL(updateLocalStatus(QString)),
                this, SLOT(updateLocalStatus(QString)));
        connect(m_radioSettings, SIGNAL(updateRemoteStatus(QString)),
                this, SLOT(updateRemoteStatus(QString)));
        connect(m_radioSettings, SIGNAL(updateLocalRssi(QString)),
                this, SLOT(updateLocalRssi(QString)));
        connect(m_radioSettings, SIGNAL(updateRemoteRssi(QString)),
                this, SLOT(updateRemoteRssi(QString)));
    }

    if(m_radioSettings->openSerialPort(m_settings)){
         m_timer->stop(); // Stop updatuing the ports combobox

        m_radioSettings->writeEscapeSeqeunce(); // Start Sate machine
    }
}

void Radio3DRConfig::updateLocalStatus(QString status)
{
    QLOG_DEBUG() << "local status:" << status;
    ui.localStatus->setText(tr("<b>STATUS:</b> ") + status);
}

void Radio3DRConfig::updateRemoteStatus(QString status)
{
    QLOG_DEBUG() << "remote status:" << status;
    ui.remoteStatus->setText(tr("<b>STATUS:</b> ") + status);
}

void Radio3DRConfig::localReadComplete(Radio3DREeprom& eeprom, bool success)
{
    QLOG_INFO() << "local Radio Read Complete success:" << success;
    if (success){
        ui.versionLabel->setText(eeprom.versionString());

        ui.formatLineEdit->setText(QString::number(eeprom.eepromVersion()));
        ui.baudComboBox->setCurrentIndex(ui.baudComboBox->findData(eeprom.serialSpeed()));
        ui.airBaudComboBox->setCurrentIndex(ui.airBaudComboBox->findData(eeprom.airSpeed()));
        ui.netIdSpinBox->setValue(eeprom.netID());
        ui.txPowerComboBox->setCurrentIndex(ui.txPowerComboBox->findData(eeprom.txPower()));
        ui.eccCheckBox->setChecked(eeprom.ecc());
        ui.mavLinkCheckBox->setChecked(eeprom.mavlink());
        ui.opResendCheckBox->setChecked(eeprom.oppResend());

        ui.dutyCycleSpinBox->setValue(eeprom.dutyCyle());
        ui.lbtRssiSpinBox->setValue(eeprom.lbtRssi());
        ui.numChannelsSpinBox->setValue(eeprom.numChannels());

        setupFrequencyComboBox(*ui.minFreqComboBox, eeprom.frequencyCode());
        setupFrequencyComboBox(*ui.maxFreqComboBox, eeprom.frequencyCode());
        ui.minFreqComboBox->setCurrentIndex(ui.minFreqComboBox->findData(eeprom.minFreq()));
        ui.maxFreqComboBox->setCurrentIndex(ui.maxFreqComboBox->findData(eeprom.maxFreq()));

    }
}

void Radio3DRConfig::remoteReadComplete(Radio3DREeprom& eeprom, bool success)
{
    QLOG_INFO() << "remote Radio Read Complete success:" << success;
    if (success){
        ui.versionLabel_remote->setText(eeprom.versionString());

        ui.formatLineEdit_remote->setText(QString::number(eeprom.eepromVersion()));
        ui.baudComboBox_remote->setCurrentIndex(ui.baudComboBox_remote->findData(eeprom.serialSpeed()));
        ui.airBaudComboBox_remote->setCurrentIndex(ui.airBaudComboBox_remote->findData(eeprom.airSpeed()));
        ui.netIdSpinBox_remote->setValue(eeprom.netID());
        ui.txPowerComboBox_remote->setCurrentIndex(ui.txPowerComboBox_remote->findData(eeprom.txPower()));
        ui.eccCheckBox_remote->setChecked(eeprom.ecc());
        ui.mavLinkCheckBox_remote->setChecked(eeprom.mavlink());
        ui.opResendCheckBox_remote->setChecked(eeprom.oppResend());

        ui.dutyCycleSpinBox_remote->setValue(eeprom.dutyCyle());
        ui.lbtRssiSpinBox_remote->setValue(eeprom.lbtRssi());
        ui.numChannelsSpinBox_remote->setValue(eeprom.numChannels());

        setupFrequencyComboBox(*ui.minFreqComboBox_remote, eeprom.frequencyCode());
        setupFrequencyComboBox(*ui.maxFreqComboBox_remote, eeprom.frequencyCode());
        ui.minFreqComboBox_remote->setCurrentIndex(ui.minFreqComboBox_remote->findData(eeprom.minFreq()));
        ui.maxFreqComboBox_remote->setCurrentIndex(ui.maxFreqComboBox_remote->findData(eeprom.maxFreq()));

    }

}


void Radio3DRConfig::writeLocalRadioSettings()
{
    QLOG_INFO() << "save 3DR Local Radio Settings";

    if(m_radioSettings == NULL){
        QMessageBox::critical(this, tr("Radio Config"), tr("Please Load Settings before attempting to change them"));
        return;
    }

    m_newRadioSettings.airSpeed(ui.airBaudComboBox->itemData(ui.airBaudComboBox->currentIndex()).toInt());
    m_newRadioSettings.serialSpeed(ui.baudComboBox->itemData(ui.baudComboBox->currentIndex()).toInt());
    m_newRadioSettings.netID(ui.netIdSpinBox->value());
    m_newRadioSettings.txPower(ui.txPowerComboBox->itemData(ui.txPowerComboBox->currentIndex()).toInt());
    m_newRadioSettings.ecc(ui.eccCheckBox->checkState() == Qt::Checked?1:0);
    m_newRadioSettings.mavlink(ui.mavLinkCheckBox->checkState() == Qt::Checked?1:0);
    m_newRadioSettings.oppResend(ui.opResendCheckBox->checkState() == Qt::Checked?1:0);
    m_newRadioSettings.minFreq(ui.minFreqComboBox->itemData(ui.minFreqComboBox->currentIndex()).toInt());
    m_newRadioSettings.maxFreq(ui.maxFreqComboBox->itemData(ui.maxFreqComboBox->currentIndex()).toInt());
    m_newRadioSettings.numChannels(ui.numChannelsSpinBox->value());
    m_newRadioSettings.dutyCyle(ui.dutyCycleSpinBox->value());
    m_newRadioSettings.lbtRssi(ui.lbtRssiSpinBox->value());
    m_newRadioSettings.manchester(0);
    m_newRadioSettings.rtsCts(0);

    m_radioSettings->writeLocalSettings(m_newRadioSettings);

}

void Radio3DRConfig::writeRemoteRadioSettings()
{
    QLOG_INFO() << "save 3DR Remote Radio Settings";

    if(m_radioSettings == NULL){
        QMessageBox::critical(this, tr("Radio Config"), tr("Please Load Settings before attempting to change them"));
        return;
    }

    m_newRadioSettings.airSpeed(ui.airBaudComboBox_remote->itemData(ui.airBaudComboBox_remote->currentIndex()).toInt());
    m_newRadioSettings.serialSpeed(ui.baudComboBox_remote->itemData(ui.baudComboBox_remote->currentIndex()).toInt());
    m_newRadioSettings.netID(ui.netIdSpinBox_remote->value());
    m_newRadioSettings.txPower(ui.txPowerComboBox_remote->itemData(ui.txPowerComboBox_remote->currentIndex()).toInt());
    m_newRadioSettings.ecc(ui.eccCheckBox_remote->checkState() == Qt::Checked?1:0);
    m_newRadioSettings.mavlink(ui.mavLinkCheckBox_remote->checkState() == Qt::Checked?1:0);
    m_newRadioSettings.oppResend(ui.opResendCheckBox_remote->checkState() == Qt::Checked?1:0);
    m_newRadioSettings.minFreq(ui.minFreqComboBox_remote->itemData(ui.minFreqComboBox_remote->currentIndex()).toInt());
    m_newRadioSettings.maxFreq(ui.maxFreqComboBox_remote->itemData(ui.maxFreqComboBox_remote->currentIndex()).toInt());
    m_newRadioSettings.numChannels(ui.numChannelsSpinBox_remote->value());
    m_newRadioSettings.dutyCyle(ui.dutyCycleSpinBox_remote->value());
    m_newRadioSettings.lbtRssi(ui.lbtRssiSpinBox_remote->value());
    m_newRadioSettings.manchester(0);
    m_newRadioSettings.rtsCts(0);

    m_radioSettings->writeRemoteSettings(m_newRadioSettings);

}

void Radio3DRConfig::copyLocalSettingsToRemote()
{
    QLOG_INFO() << "Copy 3DR Local Radio Settings to remote";
    if(m_radioSettings == NULL){
        QMessageBox::critical(this, tr("Radio Config"), tr("Please Load Settings before attempting to change them"));
        return;
    }

    ui.baudComboBox_remote->setCurrentIndex(ui.baudComboBox->currentIndex());
    ui.airBaudComboBox_remote->setCurrentIndex(ui.airBaudComboBox->currentIndex());
    ui.netIdSpinBox_remote->setValue(ui.netIdSpinBox->value());
    ui.txPowerComboBox_remote->setCurrentIndex(ui.txPowerComboBox->currentIndex());
    ui.eccCheckBox_remote->setChecked(ui.eccCheckBox->checkState());
    ui.mavLinkCheckBox_remote->setChecked(ui.mavLinkCheckBox->checkState());
    ui.opResendCheckBox_remote->setChecked(ui.opResendCheckBox->checkState());

    ui.dutyCycleSpinBox_remote->setValue(ui.dutyCycleSpinBox->value());
    ui.lbtRssiSpinBox_remote->setValue(ui.lbtRssiSpinBox->value());
    ui.numChannelsSpinBox_remote->setValue(ui.numChannelsSpinBox->value());

    ui.minFreqComboBox_remote->setCurrentIndex(ui.minFreqComboBox->currentIndex());
    ui.maxFreqComboBox_remote->setCurrentIndex(ui.maxFreqComboBox->currentIndex());
}

void Radio3DRConfig::updateLocalRssi(QString status)
{
    ui.rssiTextEdit->setText(status);
}

void Radio3DRConfig::updateRemoteRssi(QString status)
{
    ui.rssiTextEdit_remote->setText(status);
}

void Radio3DRConfig::addRadioBaudComboBoxConfig(QComboBox &comboBox)
{
    comboBox.addItem(QLatin1String("115200"), 115 );
    comboBox.addItem(QLatin1String("57600"), 57 );
    comboBox.addItem(QLatin1String("38400"), 38 );
    comboBox.addItem(QLatin1String("19200"), 19 );
    comboBox.addItem(QLatin1String("9600"), 9 );
    comboBox.addItem(QLatin1String("4800"), 4 );
    comboBox.addItem(QLatin1String("2400"), 2 );
    comboBox.addItem(QLatin1String("1200"), 1 );

}

void Radio3DRConfig::addRadioAirBaudComboBoxConfig(QComboBox &comboBox)
{
    comboBox.addItem(QLatin1String("250000"), 250);
    comboBox.addItem(QLatin1String("192000"), 192 );
    comboBox.addItem(QLatin1String("128000"), 128 );
    comboBox.addItem(QLatin1String("96000"), 96 );
    comboBox.addItem(QLatin1String("64000"), 64 );
    comboBox.addItem(QLatin1String("32000"), 32 );
    comboBox.addItem(QLatin1String("16000"), 16 );
    comboBox.addItem(QLatin1String("8000"), 8 );
    comboBox.addItem(QLatin1String("4000"), 4 );
    comboBox.addItem(QLatin1String("2000"), 2 );
}

void Radio3DRConfig::addTxPowerComboBoxConfig(QComboBox &comboBox)
{
    comboBox.addItem(QLatin1String("1dBm"), 1); // (1.3mW)
    comboBox.addItem(QLatin1String("2dBm"), 2); //  (1.6mW)
    comboBox.addItem(QLatin1String("5dBm"), 5); // (3.2mW)
    comboBox.addItem(QLatin1String("8dBm"), 8); // (6.3mW)
    comboBox.addItem(QLatin1String("11dBm"), 11); // (12.5mW)
    comboBox.addItem(QLatin1String("14dBm"), 14); //  (25mW)
    comboBox.addItem(QLatin1String("17dBm"), 17); // (50mW)
    comboBox.addItem(QLatin1String("20dBm"), 20); //  (100mW)
}

void Radio3DRConfig::setupFrequencyComboBox(QComboBox &comboBox, int freqCode )
{
    int minFreq;
    int maxFreq;
    int freqStepSize;

    switch(freqCode){
    case FREQ_915:
        minFreq = 895000;
        maxFreq = 935000;
        freqStepSize = 1000;
        break;
    case FREQ_433:
        minFreq = 414000;
        maxFreq = 454000;
        freqStepSize = 100;
        break;
    case FREQ_868:
        minFreq = 849000;
        maxFreq = 889000;
        freqStepSize = 1000;
    default:
        minFreq = 1;    // this supports RFD900 and RFD900A
        maxFreq = 30;
        freqStepSize = 1;
    }
    for (int freq = minFreq; freq <= maxFreq; freq = freq + freqStepSize) {
        comboBox.addItem(QString::number(freq), freq);
    }
}

void Radio3DRConfig::resetRadioSettingsToDefaults()
{
    if(m_radioSettings) {

        if (QMessageBox::warning(this, tr("Reset Radios"), tr("You are about to reset your radios to thier factory settings!"),
                             QMessageBox::Ok, QMessageBox::Cancel) == QMessageBox::Ok){
            m_radioSettings->resetToDefaults();
        }
    }
}
