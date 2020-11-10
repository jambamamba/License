#include "License.h"

#include <openssl/ossl_typ.h>
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/err.h>

#include <QByteArray>
#include <QFile>
#include <QStringList>
#include <QXmlStreamReader>

extern "C" void mylog(const char *fmt, ...);

License::License()
{
}

QByteArray License::loadPrivateKey() const
{
    QFile privateKeyResource(":/resources/private.key");
    if(!privateKeyResource.open(QIODevice::ReadOnly))
    {
        mylog("Failed to load private key");
        return QByteArray();
    }
    QByteArray privateKey = privateKeyResource.readAll();
    privateKeyResource.close();
    if(!privateKey.size())
    {
        mylog( "Could not load private key" );
        return QByteArray();
    }
    return privateKey;
}

int License::getRsaEncryptedSecret(const QByteArray &license, const QByteArray &privateKey) const
{
    const unsigned char* keyData = (const unsigned char*)privateKey.data();
    RSA * rsa = d2i_RSAPrivateKey(NULL, &keyData, privateKey.size());
    if(!rsa)
    {
        mylog ("d2i_RSAPrivateKey failed");
        return 0;
    }
    const int xorEncryptedStringSize = 512;
    unsigned char rsa_decrypted_string[64];

    ERR_load_crypto_strings();
    int keySize = license.size() - xorEncryptedStringSize;
    int iret = RSA_private_decrypt(keySize,
                                   (const unsigned char*)license.data(),
                                   rsa_decrypted_string,
                                   rsa,
                                   RSA_PKCS1_PADDING);
    if(-1==iret)
    {
        unsigned long err = ERR_get_error();
        char* errs = ERR_error_string(err, NULL);
        if(!errs)
        {
            mylog("RSA_private_decrypt failed with error: %i", err);
            return 0;
        }
        else
        {
            mylog("RSA_private_decrypt failed with error: %i:%s", err, errs);
            return 0;
        }
    }
    else
    {
        return atoi((const char*)rsa_decrypted_string);
    }
    ERR_free_strings();
    RSA_free(rsa);
}

QString License::xmlDataFromLicense(int secret, const QByteArray& license) const
{
    QString xml;
    for(int i = 256; i < license.size(); i+=2)
    {
        char hexdigit[3];
        hexdigit[0] = license.at(i);
        hexdigit[1] = license.at(i+1);
        hexdigit[2] = 0;
        char* ptr = 0;
        unsigned long byte = strtoul(hexdigit, &ptr, 16);
        byte ^= secret;
        xml.append(QString("%1").arg((char)byte));
    }

    return xml;
}

QPair<LicenseType, QString> License::parseDataFromLicenseXmlString(const QStringList &verificationString, const QString &xml) const
{
    QString buyer_email;
    int verification_string_index = -1;

    QXmlStreamReader reader(xml);
    while (!reader.atEnd())
    {
        reader.readNext();
        if(reader.isStartElement())
        {
            if(reader.name() == "verification_string")
            {
                QString elementValue = reader.readElementText();
                for(int i=0; i< verificationString.size() && verification_string_index==-1; ++i)
                {
                    if(verificationString[i].compare(elementValue) == 0)
                    {
                        verification_string_index = i;
                    }
                }
            }
            else if(reader.name() == "buyer_email")
            {
                buyer_email = reader.readElementText();
            }
        }
    }
    return verification_string_index == -1 ? QPair<LicenseType, QString>(LicenseType_Invalid, buyer_email) :
                                             QPair<LicenseType, QString>((LicenseType)verification_string_index, buyer_email);
}

QPair<LicenseType, QString> License::validate(const QByteArray& license) const
{
    QString buyerEmail;
    if(!license.size())
    {
        mylog("there is no license");
        return QPair<LicenseType, QString>(LicenseType_Invalid, buyerEmail);
    }

    QStringList verificationString;
    for(int i = 0; i < LicenseType_Max; ++i)
    {
        verificationString << LICENSE_TYPE[i];
    }

    QByteArray privateKey = loadPrivateKey();
    if(!privateKey.size())
    {
        return QPair<LicenseType, QString>(LicenseType_Invalid, buyerEmail);
    }

    int secret = getRsaEncryptedSecret(license, privateKey);
    if(secret < 1)
    {
        return QPair<LicenseType, QString>(LicenseType_Invalid, buyerEmail);
    }

    QString xml = xmlDataFromLicense(secret, license);
    //mylog("%s", xml.toUtf8().data());

    //todo : parse buyer_email and verification_string out of the xml
    return parseDataFromLicenseXmlString(verificationString, xml);
}
