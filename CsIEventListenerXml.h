/*
 * ©2015 Symbol Technologies LLC. All rights reserved.
 */

#ifndef IEVENTLISTENERXML_H
#define IEVENTLISTENERXML_H

#include <string>

class IEventListenerXml
{
	/// Any client application must implement these virtual methods in order to get events
public:

    virtual void OnImageEvent( short eventType, int size, short imageFormat, char* sfimageData, int dataLength, std::string& pScannerData ) = 0;
    virtual void OnImageEvent( short eventType, int size , short imageFormat, char *sfimageData, std::string& pScannerData) 
    {
        // if no new method implemented then call the depricated method //
        this->OnImageEvent( eventType , size , imageFormat, sfimageData , size , pScannerData );    
    }


    virtual void OnVideoEvent( short eventType, int size, char* sfvideoData, int dataLength, std::string& pScannerData ) = 0;
    virtual void OnBarcodeEvent( short eventType, std::string& pscanData ) = 0;
    virtual void OnPNPEvent( short eventType, std::string ppnpData ) = 0;
    virtual void OnCommandResponseEvent( short status, std::string& prspData ) = 0;
    virtual void OnScannerNotification( short notificationType, std::string& pScannerData ) = 0;
    virtual void OnIOEvent( short type, unsigned char data ) = 0;
    virtual void OnScanRMDEvent( short eventType, std::string& prmdData ) = 0;
    virtual void OnDisconnect() = 0;

};


#endif // IEVENTLISTENERXML_H

