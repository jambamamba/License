#include <QDesktopServices>
#include <QDir>
#include <QDebug>

#include "LicenseWidget.h"
#include "LicenseComm.h"
#include "ui_LicenseWidget.h"

extern "C" void mylog(const char *fmt, ...);

LicenseWidget::LicenseWidget(const QString app_name, int app_id, int version,
                             QUuid &guid, const QString &order_page_url, QWidget *parent)
    : QWidget(parent)
    , m_onLicenseValidated(nullptr)
    , ui(new Ui::GetLicenseWidget)
    , m_license_comm(new LicenseComm(app_name, app_id, version, guid, this))
    , m_order_page_url(order_page_url)
{
    ui->setupUi(this);
    ui->licenseTerminatedContainerWidget->setVisible(false);
    connect(m_license_comm, &LicenseComm::validate,
            this, &LicenseWidget::validate);
    connect(m_license_comm, &LicenseComm::terminated,
            this, &LicenseWidget::terminated);
    makePushButtonIntoWebLink();
}

LicenseWidget::~LicenseWidget()
{
    delete ui;
}

void LicenseWidget::checkLicense(const QString &label, std::function<void(int)>onLicenseValidated)
{
    m_onLicenseValidated = onLicenseValidated;
    ui->getLicenseLabel->setText(label);
    QString url = m_order_page_url;
    url.truncate(30);
    ui->openLicenseWebPagePushButton->setText(url.append("..."));

    loadLicenseFromFile();
    validate(getLicense());

    m_license_comm->checkLicense(ui->userEmail->text());
}

void LicenseWidget::addAboutAppWidget(QWidget *widget)
{
    ui->aboutAppHorizontalLayout->addWidget(widget);
}

QString LicenseWidget::openLicenseWebPagePushButtonText()
{
    return m_order_page_url;// ui->openLicenseWebPagePushButton->text();
}

QByteArray LicenseWidget::getLicense()
{
    return ui->licenseKey->toPlainText().toUtf8();
}

void LicenseWidget::setLicenseValid(const QString &buyerEmail)
{
    ui->noLicenseContainerWidget->hide();
    ui->userEmail->setText(buyerEmail);
    ui->userEmail->show();
    ui->userEmail->setStyleSheet("QLabel {color: blue; }");
    ui->userEmailLabel->show();
}

void LicenseWidget::setLicenseInvalid()
{
    ui->validateLicensePushButton->show();
    ui->licenseKey->setEnabled(true);
    ui->userEmail->hide();
    ui->userEmailLabel->hide();
    ui->getLicenseLabel->show();
    ui->openLicenseWebPagePushButton->show();
}

void LicenseWidget::showLicense(const QString &license)
{
    ui->licenseKey->setPlainText(license);
}

void LicenseWidget::makePushButtonIntoWebLink()
{
    ui->openLicenseWebPagePushButton->setStyleSheet("QPushButton {color: blue; text-decoration:underline; }");
    ui->openLicenseWebPagePushButton->setCursor(Qt::PointingHandCursor);
}

void LicenseWidget::on_openLicenseWebPagePushButton_clicked()
{
    auto url = openLicenseWebPagePushButtonText();
    url = url.replace("&&", "&");
    QDesktopServices::openUrl(QUrl(url));
}

void LicenseWidget::on_validateLicensePushButton_clicked()
{
    validate(getLicense());
}

void LicenseWidget::on_pushButtonDownloadLicense_clicked()
{
    m_license_comm->checkLicense(ui->userEmail->text());
}

void LicenseWidget::validate(const QByteArray &lic)
{
    QByteArray license = QByteArray::fromBase64(lic);
    if(license.isEmpty())
    {
        setLicenseInvalid();
        return;
    }
    QPair<LicenseType, QString> pair = m_license.validate(license.trimmed());
    LicenseType index = pair.first;
    QString buyerEmail = pair.second;

    if(!m_onLicenseValidated) { mylog("m_onLicenseValidated"); }
    m_onLicenseValidated(index);
    if(index > LicenseType_Invalid)
    {
        saveLicenseToFile(license.trimmed().toBase64());
        showLicense(license.trimmed().toBase64());
        setLicenseValid(buyerEmail);
    }
    else
    {
       setLicenseInvalid();
    }
}

void LicenseWidget::terminated(const QString &msg)
{
    setLicenseInvalid();
    ui->licenseKey->setPlainText("");
    ui->validateLicensePushButton->setEnabled(false);
    ui->pushButtonDownloadLicense->setEnabled(false);

    ui->noLicenseContainerWidget->hide();
    ui->labelTerminatedMessage->setText(msg);
    ui->licenseTerminatedContainerWidget->show();
}

void LicenseWidget::loadLicenseFromFile()
{
    QFile licenseFile(m_license_comm->licenseKeyPath());
    if(licenseFile.open(QIODevice::ReadOnly))
    {
        QString license(licenseFile.readAll());
        licenseFile.close();
        showLicense(license);
    }
}

QString LicenseWidget::userEmail() const
{
    return ui->userEmail->text();
}

void LicenseWidget::saveLicenseToFile(const QByteArray &license)
{
    QFile licenseFile(m_license_comm->licenseKeyPath());
    if(licenseFile.open(QIODevice::WriteOnly))
    {
        licenseFile.write(license);
        licenseFile.close();
    }
}
