//
//  nexdome.cpp
//  NexDome X2 plugin
//
//  Created by Rodolphe Pineau on 6/11/2016.


#include "aaf2.h"
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <memory.h>
#include <string.h>
#ifdef SB_MAC_BUILD
#include <unistd.h>
#endif
#ifdef SB_WIN_BUILD
#include <time.h>
#endif


CAaf2Controller::CAaf2Controller()
{

    m_pSerx = NULL;
    m_pLogger = NULL;


    m_bDebugLog = false;
    m_bIsConnected = false;

    m_nCurPos = 0;
    m_nTargetPos = 0;
    m_nPosLimit = 0;
    m_bPosLimitEnabled = 0;
    m_bMoving = false;


#ifdef	AAF2_DEBUG
	Logfile = fopen(AAF2_LOGFILENAME, "w");
	ltime = time(NULL);
	char *timestamp = asctime(localtime(&ltime));
	timestamp[strlen(timestamp) - 1] = 0;
	fprintf(Logfile, "[%s] CAaf2Controller Constructor Called.\n", timestamp);
	fflush(Logfile);
#endif

}

CAaf2Controller::~CAaf2Controller()
{
#ifdef	AAF2_DEBUG
    // Close LogFile
    if (Logfile) fclose(Logfile);
#endif
}

int CAaf2Controller::Connect(const char *pszPort)
{
    int nErr = AAF2_OK;
    int nStatus;

    if(!m_pSerx)
        return ERR_COMMNOLINK;

#ifdef AAF2_DEBUG
	ltime = time(NULL);
	timestamp = asctime(localtime(&ltime));
	timestamp[strlen(timestamp) - 1] = 0;
	fprintf(Logfile, "[%s] CAaf2Controller::Connect Called %s\n", timestamp, pszPort);
	fflush(Logfile);
#endif

    // 9600 8N1
    nErr = m_pSerx->open(pszPort, 9600, SerXInterface::B_NOPARITY, "-DTR_CONTROL 1");
    if( nErr == 0)
        m_bIsConnected = true;
    else
        m_bIsConnected = false;

    if(!m_bIsConnected)
        return nErr;

    m_pSleeper->sleep(2000);

#ifdef AAF2_DEBUG
	ltime = time(NULL);
	timestamp = asctime(localtime(&ltime));
	timestamp[strlen(timestamp) - 1] = 0;
	fprintf(Logfile, "[%s] CAaf2Controller::Connect connected to %s\n", timestamp, pszPort);
	fflush(Logfile);
#endif
	
    if (m_bDebugLog && m_pLogger) {
        snprintf(m_szLogBuffer,LOG_BUFFER_SIZE,"[CAaf2Controller::Connect] Connected.\n");
        m_pLogger->out(m_szLogBuffer);

        snprintf(m_szLogBuffer,LOG_BUFFER_SIZE,"[CAaf2Controller::Connect] Getting Firmware.\n");
        m_pLogger->out(m_szLogBuffer);
    }

    // get status so we can figure out what device we are connecting to.
#ifdef AAF2_DEBUG
	ltime = time(NULL);
	timestamp = asctime(localtime(&ltime));
	timestamp[strlen(timestamp) - 1] = 0;
	fprintf(Logfile, "[%s] CAaf2Controller::Connect getting device status\n", timestamp);
	fflush(Logfile);
#endif
    nErr = getDeviceStatus(nStatus);
    if(nErr) {
		m_bIsConnected = false;
#ifdef AAF2_DEBUG
		ltime = time(NULL);
		timestamp = asctime(localtime(&ltime));
		timestamp[strlen(timestamp) - 1] = 0;
		fprintf(Logfile, "[%s] CAaf2Controller::Connect **** ERROR **** getting device status\n", timestamp);
		fflush(Logfile);
#endif
        return nErr;
    }
    // m_globalStatus.deviceType now contains the device type
    return nErr;
}

void CAaf2Controller::Disconnect()
{
    if(m_bIsConnected && m_pSerx)
        m_pSerx->close();
 
	m_bIsConnected = false;
}

#pragma mark move commands
int CAaf2Controller::haltFocuser()
{
    int nErr;
    char szResp[SERIAL_BUFFER_SIZE];
    char szTmpBuf[SERIAL_BUFFER_SIZE];

	if(!m_bIsConnected)
		return ERR_COMMNOLINK;

    nErr = aaf2Command("H#", szResp, SERIAL_BUFFER_SIZE);
    if(nErr)
        return nErr;
    // parse output to update m_curPos
    parseResponse(szResp, szTmpBuf, SERIAL_BUFFER_SIZE);
    m_nCurPos = atoi(szTmpBuf);
    m_nTargetPos = m_nCurPos;

    return nErr;
}

int CAaf2Controller::gotoPosition(int nPos)
{
    int nErr;
    char szCmd[SERIAL_BUFFER_SIZE];
    char szResp[SERIAL_BUFFER_SIZE];
    char szTmpBuf[SERIAL_BUFFER_SIZE];

	if(!m_bIsConnected)
		return ERR_COMMNOLINK;


    if (m_bPosLimitEnabled && nPos>m_nPosLimit)
        return ERR_LIMITSEXCEEDED;

#ifdef AAF2_DEBUG
    ltime = time(NULL);
    timestamp = asctime(localtime(&ltime));
    timestamp[strlen(timestamp) - 1] = 0;
    fprintf(Logfile, "[%s] CAaf2Controller::gotoPosition goto position  : %d\n", timestamp, nPos);
    fflush(Logfile);
#endif

    snprintf(szCmd, SERIAL_BUFFER_SIZE, "T%d#", nPos);
    nErr = aaf2Command(szCmd, szResp, SERIAL_BUFFER_SIZE);
    if(nErr)
        return nErr;
    if(!strstr(szResp,"OK"))
        return ERR_CMDFAILED;

    nErr = parseResponse(szResp, szTmpBuf, SERIAL_BUFFER_SIZE);
    m_nTargetPos = atoi(szTmpBuf);

    return nErr;
}

int CAaf2Controller::moveRelativeToPosision(int nSteps)
{
    int nErr;

	if(!m_bIsConnected)
		return ERR_COMMNOLINK;

#ifdef AAF2_DEBUG
    ltime = time(NULL);
    timestamp = asctime(localtime(&ltime));
    timestamp[strlen(timestamp) - 1] = 0;
    fprintf(Logfile, "[%s] CAaf2Controller::gotoPosition goto relative position  : %d\n", timestamp, nSteps);
    fflush(Logfile);
#endif

    m_nTargetPos = m_nCurPos + nSteps;
    nErr = gotoPosition(m_nTargetPos);
    return nErr;
}

#pragma mark command complete functions

int CAaf2Controller::isGoToComplete(bool &bComplete)
{
    int nErr = AAF2_OK;
	
	if(!m_bIsConnected)
		return ERR_COMMNOLINK;

    getPosition(m_nCurPos);
    if(m_nCurPos == m_nTargetPos)
        bComplete = true;
    else
        bComplete = false;
    return nErr;
}

int CAaf2Controller::isMotorMoving(bool &bMoving)
{
    int nErr = AAF2_OK;
    char szResp[SERIAL_BUFFER_SIZE];
	
	if(!m_bIsConnected)
		return ERR_COMMNOLINK;

    nErr = aaf2Command("M#", szResp, SERIAL_BUFFER_SIZE);
    if(nErr)
        return nErr;
    if(strstr(szResp,"OK")) { // positive response, check M value
        if(strstr(szResp,"M0")) {
            bMoving = false;
        }
        else if (strstr(szResp,"M1")) {
            bMoving = true;
        }
        else
            nErr = ERR_CMDFAILED;
    }

    return nErr;
}

#pragma mark getters and setters
int CAaf2Controller::getDeviceStatus(int &nStatus)
{
    int nErr;
    char szResp[SERIAL_BUFFER_SIZE];
	
	if(!m_bIsConnected)
		return ERR_COMMNOLINK;
	

    nErr = aaf2Command("#", szResp, SERIAL_BUFFER_SIZE);
    if(nErr)
        return nErr;

    if(strstr(szResp,"OK")) {
        nStatus = AAF2_OK;
        nErr = AAF2_OK;
    }
    else {
        nStatus = COMMAND_FAILED;
        nErr = ERR_CMDFAILED;
    }
    return nErr;
}

int CAaf2Controller::getFirmwareVersion(char *pszVersion, int nStrMaxLen)
{
    int nErr = AAF2_OK;
    char szResp[SERIAL_BUFFER_SIZE];
	
	if(!m_bIsConnected)
		return ERR_COMMNOLINK;

    if(!m_bIsConnected)
        return NOT_CONNECTED;

    nErr = aaf2Command("V#", szResp, SERIAL_BUFFER_SIZE);
    if(nErr)
        return nErr;
#ifdef AAF2_DEBUG
    ltime = time(NULL);
    timestamp = asctime(localtime(&ltime));
    timestamp[strlen(timestamp) - 1] = 0;
    fprintf(Logfile, "[%s] CAaf2Controller::getFirmwareVersion szResp : %s\n", timestamp, szResp);
    fflush(Logfile);
#endif

    strncpy(pszVersion, szResp, nStrMaxLen);
    return nErr;
}

int CAaf2Controller::getTemperature(double &dTemperature)
{
    int nErr = AAF2_OK;
    char szResp[SERIAL_BUFFER_SIZE];
    char szTmpBuf[SERIAL_BUFFER_SIZE];

	if(!m_bIsConnected)
		return ERR_COMMNOLINK;

    nErr = aaf2Command("C#", szResp, SERIAL_BUFFER_SIZE);
    if(nErr)
        return nErr;

    // parse output to extract temp value.
    nErr = parseResponse(szResp, szTmpBuf, SERIAL_BUFFER_SIZE);
    // convert string value to double
    dTemperature = atof(szTmpBuf);

    return nErr;
}

int CAaf2Controller::getPosition(int &nPosition)
{
    int nErr = AAF2_OK;
    char szResp[SERIAL_BUFFER_SIZE];
    char szTmpBuf[SERIAL_BUFFER_SIZE];

	if(!m_bIsConnected)
		return ERR_COMMNOLINK;

    nErr = aaf2Command("P#", szResp, SERIAL_BUFFER_SIZE);
    if(nErr)
        return nErr;

    // parse output to extract position value.
    nErr = parseResponse(szResp, szTmpBuf, SERIAL_BUFFER_SIZE);
    // convert response
    nPosition = atoi(szTmpBuf);
    m_nCurPos = nPosition;

    return nErr;
}


int CAaf2Controller::syncMotorPosition(int nPos)
{
    int nErr = AAF2_OK;
    char szCmd[SERIAL_BUFFER_SIZE];
    char szResp[SERIAL_BUFFER_SIZE];

	if(!m_bIsConnected)
		return ERR_COMMNOLINK;

    snprintf(szCmd, SERIAL_BUFFER_SIZE, "I%d#", nPos);
    printf("setting new pos to %d [ %s ]\n",nPos, szCmd);
    
    nErr = aaf2Command(szCmd, szResp, SERIAL_BUFFER_SIZE);
    printf("[syncMotorPosition] szResp = %s\n", szResp);
    if(nErr)
        return nErr;

    if(!strstr(szResp,"OK"))
        nErr = ERR_CMDFAILED;
    m_nCurPos = nPos;
    return nErr;
}



int CAaf2Controller::getPosLimit()
{
    return m_nPosLimit;
}

void CAaf2Controller::setPosLimit(int nLimit)
{
    m_nPosLimit = nLimit;
}

bool CAaf2Controller::isPosLimitEnabled()
{
    return m_bPosLimitEnabled;
}

void CAaf2Controller::enablePosLimit(bool bEnable)
{
    m_bPosLimitEnabled = bEnable;
}



#pragma mark command and response functions

int CAaf2Controller::aaf2Command(const char *pszszCmd, char *pszResult, int nResultMaxLen)
{
    int nErr = AAF2_OK;
    char szResp[SERIAL_BUFFER_SIZE];
    unsigned long  ulBytesWrite;
	
	if(!m_bIsConnected)
		return ERR_COMMNOLINK;

    m_pSerx->purgeTxRx();
    if (m_bDebugLog && m_pLogger) {
        snprintf(m_szLogBuffer,LOG_BUFFER_SIZE,"[CAaf2Controller::aaf2Command] Sending %s\n",pszszCmd);
        m_pLogger->out(m_szLogBuffer);
    }
#ifdef AAF2_DEBUG
	ltime = time(NULL);
	timestamp = asctime(localtime(&ltime));
	timestamp[strlen(timestamp) - 1] = 0;
	fprintf(Logfile, "[%s] CAaf2Controller::aaf2Command Sending %s\n", timestamp, pszszCmd);
	fflush(Logfile);
#endif
    nErr = m_pSerx->writeFile((void *)pszszCmd, strlen(pszszCmd), ulBytesWrite);
    m_pSerx->flushTx();

    if(nErr){
        if (m_bDebugLog && m_pLogger) {
            snprintf(m_szLogBuffer,LOG_BUFFER_SIZE,"[CAaf2Controller::aaf2Command] writeFile Error.\n");
            m_pLogger->out(m_szLogBuffer);
        }
        return nErr;
    }

    if(pszResult) {
        // read response
        if (m_bDebugLog && m_pLogger) {
            snprintf(m_szLogBuffer,LOG_BUFFER_SIZE,"[CAaf2Controller::aaf2Command] Getting response.\n");
            m_pLogger->out(m_szLogBuffer);
        }
        nErr = readResponse(szResp, SERIAL_BUFFER_SIZE);
        if(nErr){
            if (m_bDebugLog && m_pLogger) {
                snprintf(m_szLogBuffer,LOG_BUFFER_SIZE,"[CAaf2Controller::aaf2Command] readResponse Error.\n");
                m_pLogger->out(m_szLogBuffer);
            }
        }
#ifdef AAF2_DEBUG
		ltime = time(NULL);
		timestamp = asctime(localtime(&ltime));
		timestamp[strlen(timestamp) - 1] = 0;
		fprintf(Logfile, "[%s] CAaf2Controller::aaf2Command response \"%s\"\n", timestamp, szResp);
		fflush(Logfile);
#endif
        // printf("Got response : %s\n",resp);
        strncpy(pszResult, szResp, nResultMaxLen);
#ifdef AAF2_DEBUG
		ltime = time(NULL);
		timestamp = asctime(localtime(&ltime));
		timestamp[strlen(timestamp) - 1] = 0;
		fprintf(Logfile, "[%s] CAaf2Controller::aaf2Command response copied to pszResult : \"%s\"\n", timestamp, pszResult);
		fflush(Logfile);
#endif
    }
    return nErr;
}

int CAaf2Controller::readResponse(char *pszRespBuffer, int nBufferLen)
{
    int nErr = AAF2_OK;
    unsigned long ulBytesRead = 0;
    unsigned long ulTotalBytesRead = 0;
    char *pszBufPtr;
	
	if(!m_bIsConnected)
		return ERR_COMMNOLINK;

    memset(pszRespBuffer, 0, (size_t) nBufferLen);
    pszBufPtr = pszRespBuffer;

    do {
        nErr = m_pSerx->readFile(pszBufPtr, 1, ulBytesRead, MAX_TIMEOUT);
        if(nErr) {
            if (m_bDebugLog && m_pLogger) {
                snprintf(m_szLogBuffer,LOG_BUFFER_SIZE,"[CAaf2Controller::readResponse] readFile Error.\n");
                m_pLogger->out(m_szLogBuffer);
            }
            return nErr;
        }

        if (ulBytesRead !=1) {// timeout
            if (m_bDebugLog && m_pLogger) {
                snprintf(m_szLogBuffer,LOG_BUFFER_SIZE,"[CAaf2Controller::readResponse] readFile Timeout.\n");
                m_pLogger->out(m_szLogBuffer);
            }
#ifdef AAF2_DEBUG
			ltime = time(NULL);
			timestamp = asctime(localtime(&ltime));
			timestamp[strlen(timestamp) - 1] = 0;
			fprintf(Logfile, "[%s] CAaf2Controller::readResponse timeout\n", timestamp);
			fflush(Logfile);
#endif
            nErr = ERR_NORESPONSE;
            break;
        }
        ulTotalBytesRead += ulBytesRead;
        if (m_bDebugLog && m_pLogger) {
            snprintf(m_szLogBuffer,LOG_BUFFER_SIZE,"[CAaf2Controller::readResponse] ulBytesRead = %lu\n",ulBytesRead);
            m_pLogger->out(m_szLogBuffer);
        }
    } while (*pszBufPtr++ != '#' && ulTotalBytesRead < nBufferLen );

    if(ulTotalBytesRead)
        *(pszBufPtr-1) = 0; //remove the #

    return nErr;
}

int CAaf2Controller::parseResponse(char *pszResp, char *pszParsed, int nBufferLen)
{
    int nErr = AAF2_OK;
    std::string sSegment;
    std::vector<std::string> svSeglist;
    std::stringstream ssTmp(pszResp);
    std::vector<std::string>  svParsedResp;

#ifdef PEGA_DEBUG
    ltime = time(NULL);
    timestamp = asctime(localtime(&ltime));
    timestamp[strlen(timestamp) - 1] = 0;
    fprintf(Logfile, "[%s] CPegasusController::parseResp parsing \"%s\"\n", timestamp, pszResp);
    fflush(Logfile);
#endif
    svParsedResp.clear();
    // split the string into vector elements
    while(std::getline(ssTmp, sSegment, ':'))
    {
        svSeglist.push_back(sSegment);
    }
    // do we have all the fields ?
    if (svSeglist.size()<2)
        return ERR_CMDFAILED;

    svSeglist[0].erase(0, 1); // erase the first caracter.
    strncpy(pszParsed, svSeglist[0].c_str(), nBufferLen);
    return nErr;
}
