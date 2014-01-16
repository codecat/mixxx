#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <QRegExp>

#include "battery/battery.h"
#include "battery/batterylinux.h"

// these constants are used to retrieve data from infoFile and stateFile
const QString BatteryLinux::s_sMaximumCapacityKeyword = "last full capacity";
const QString BatteryLinux::s_sCurrentCapacityKeyword = "remaining capacity";
const QString BatteryLinux::s_sChargingStateKeyword = "charging state";
const QString BatteryLinux::s_sCurrentRateKeyword = "present rate";

BatteryLinux::BatteryLinux(QObject *parent, QString infoFile, QString stateFile) :
  Battery(parent),
  m_sInfoFile(infoFile),
  m_sStateFile(stateFile),
  m_iMaximumCapacity(0),
  m_iCurrentCapacity(0) {
	m_iMaximumCapacity = readMaximumCapacity();
}

BatteryLinux::~BatteryLinux() {

}

void BatteryLinux::read() {
	m_csChargingState = readChargingState();
	m_iCurrentCapacity = readCurrentCapacity();
	m_iCurrentRate = readCurrentRate();
	m_iMinutesLeft = readMinutesLeft();
	m_iPercentage = readPercentage();
}

Battery::ChargingState BatteryLinux::readChargingState() {
	QFile qfFile(m_sStateFile);
	if(!qfFile.open(QFile::ReadOnly)) {
		qDebug() << "Error opening " << m_sStateFile;
		return UNKNOWN;
	}
	QTextStream tsFile(&qfFile);
	QString sOut;
	while (!(sOut = tsFile.readLine()).isNull()) {
		if (sOut.contains(s_sChargingStateKeyword)) {
			// remove the keyword (i.e. "charging state:    ")
			sOut.remove(QRegExp(s_sChargingStateKeyword + ":*\\s+"));
			if (sOut == "discharging")
				return DISCHARGING;
			if (sOut == "charging")
				return CHARGING;
			if (sOut == "charged")
				return CHARGED;
			else
				return UNKNOWN;
		}
	}
	return UNKNOWN;
}

int BatteryLinux::readMinutesLeft() {
	if (!m_iCurrentRate) return 0; // Prevent division by 0
	switch (m_csChargingState) {
	case DISCHARGING:
		return 60 * m_iCurrentCapacity / m_iCurrentRate;
	case CHARGING:
		return 60 * (m_iMaximumCapacity - m_iCurrentCapacity) / m_iCurrentRate;
	case CHARGED:
	case UNKNOWN:
	default:
		return 0;
	}
}

int BatteryLinux::readValue(QString sFile, QString sKeyword) {
	QFile qfFile(sFile);
	if(!qfFile.open(QFile::ReadOnly)) {
		qDebug() << "Error opening " << sFile;
		return 0;
	}
	QTextStream tsFile(&qfFile);
	QString sOut;
	while (!(sOut = tsFile.readLine()).isNull()) {
		if (sOut.contains(sKeyword)) {
			// remove everything except numbers
			sOut.remove(QRegExp("\\D"));
			return sOut.toInt();
		}
	}
	return 0;
}
