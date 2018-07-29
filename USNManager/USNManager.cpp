#include "stdafx.h"
#include "USNManager.h"

/*********************************************************
名称：   CUSNManager
说明：   构造函数
入参：   
返回值： 
*********************************************************/
CUSNManager::CUSNManager()
: m_hVol(INVALID_HANDLE_VALUE)
{

}

/*********************************************************
名称：   CUSNManager
说明：   析构函数
入参：
返回值：
*********************************************************/
CUSNManager::~CUSNManager()
{
    closeUSNFile(); 
}

/*********************************************************
名称：   CUSNManager
说明：   判断驱动盘格式是否支持USN
入参：   pPath -- USN文件所在磁盘号(C,D,E,F...）
返回值： bool -- true   支持
                false 不支持
*********************************************************/
bool
CUSNManager::isUSNSupported(const char *pDriverName)
{
    printf("%s: 判断驱动盘%s是否NTFS格式\n",
        __FUNCTION__, pDriverName);

    bool status = false;
    char sysNameBuf[MAX_PATH] = { 0 };
    status = GetVolumeInformationA(
        pDriverName,
        NULL,
        0,
        NULL,
        NULL,
        NULL,
        sysNameBuf,             // 驱动盘的系统名（FAT/NTFS）
        MAX_PATH);

    if (!status) {
        printf("%s: 获取信息失败！", __FUNCTION__);
        return false;
    }

    if (0 != strcmp(sysNameBuf, "NTFS")) {
        printf("%s: 不支持USN 文件系统名: %s\n",
            __FUNCTION__, sysNameBuf);
        return false;
    }

    return true;
}

/*********************************************************
名称：   getDriverHandle
说明：   获取驱动盘的句柄
入参：   pPath -- USN文件所在磁盘号(C,D,E,F...）
返回值： HANDLE -- 句柄
*********************************************************/
HANDLE
CUSNManager::getDriverHandle(const char *pDriverName)
{
    if (!isUSNSupported(pDriverName)) {
        return INVALID_HANDLE_VALUE;
    }

    HANDLE hVol = INVALID_HANDLE_VALUE;
    printf("%s: 获取驱动盘%s 驱动盘\n",
        __FUNCTION__, pDriverName);
    char fileName[MAX_PATH] = { 0 };

    strcpy_s(fileName, "\\\\.\\");
    strcat_s(fileName, pDriverName);

    // 为了方便操作，这里转为string进行去尾  
    string fileNameStr = (string)fileName;
    fileNameStr.erase(fileNameStr.find_last_of(":") + 1);

    printf("驱动盘地址: %s", fileNameStr.data());

    // 获取句柄 调用该函数需要管理员权限
    hVol = CreateFileA(fileNameStr.data(),
        GENERIC_READ | GENERIC_WRITE,           // 可以为0  
        FILE_SHARE_READ | FILE_SHARE_WRITE,     // 必须包含有FILE_SHARE_WRITE  
        NULL, 
        OPEN_EXISTING,                          // 必须包含OPEN_EXISTING, CREATE_ALWAYS可能会导致错误  
        FILE_ATTRIBUTE_READONLY,                // FILE_ATTRIBUTE_NORMAL可能会导致错误  
        NULL); 

    return hVol;
}

/*********************************************************
名称：   getUSNBasicInfo
说明：   获取USN基本信息
入参：   hVol -- USN日志文件句柄
返回值： true  -- 获取成功
        false -- 获取失败
*********************************************************/
bool
CUSNManager::getUSNBasicInfo(HANDLE hVol)
{
    printf("%s: 获取USN基本信息，用于后续操作\n", 
        __FUNCTION__);

    bool status = false;
    DWORD br;
    status = DeviceIoControl(hVol,
        FSCTL_QUERY_USN_JOURNAL,
        NULL,
        0,
        &m_UsnInfo,
        sizeof(USN_JOURNAL_DATA),
        &br,
        NULL);

    if (!status) {
        printf("%s: 获取USN基本信息失败!\n", __FUNCTION__);
        return false;
    }

    return true;
}

/*********************************************************
名称：   openUSNFile
说明：   打开USN日志
入参：   pDriverName -- 驱动器名称
返回值： true  -- 日志打开成功
        false -- 日志打开失败
*********************************************************/
bool
CUSNManager::openUSNFile(const char *pDriverName)
{
    // 获取句柄
    m_hVol = getDriverHandle(pDriverName);
    if (INVALID_HANDLE_VALUE == m_hVol) {
        printf("%s: 获取句柄失败!\n", __FUNCTION__);
        return false;
    }

    // 初始化日志
    bool status = false;
    DWORD br;
    CREATE_USN_JOURNAL_DATA cujd;
    cujd.MaximumSize = 0;           // 0表示使用默认值  
    cujd.AllocationDelta = 0;       // 0表示使用默认值  
    status = DeviceIoControl(m_hVol,
        FSCTL_CREATE_USN_JOURNAL,
        &cujd,
        sizeof(cujd),
        NULL,
        0,
        &br,
        NULL);

    if (!status) {
        printf("%s: 初始化USN日志失败!\n", __FUNCTION__);
        return false;
    }

    // 存储基础信息
    return getUSNBasicInfo(m_hVol);
}

/*********************************************************
名称：   deleteUSNFile
说明：   删除USN存档
入参：   无
返回值： true  -- 删除成功
        false -- 删除失败
*********************************************************/
bool
CUSNManager::deleteUSNFile()
{
    if (INVALID_HANDLE_VALUE == m_hVol) {
        printf("%s: Please open one USNFile First!", 
            __FUNCTION__);
        return false;
    }

    printf("%s: 删除USN日志!\n", __FUNCTION__);

    DWORD br;
    bool status = false;
    DELETE_USN_JOURNAL_DATA dujd;
    dujd.UsnJournalID = m_UsnInfo.UsnJournalID;
    dujd.DeleteFlags = USN_DELETE_FLAG_DELETE;

    status = DeviceIoControl(m_hVol,
        FSCTL_DELETE_USN_JOURNAL,
        &dujd,
        sizeof(dujd),
        NULL,
        0,
        &br,
        NULL);

    if (!status) {
        printf("%s: USN日志删除失败！\n", __FUNCTION__);
        return false;
    }

    printf("%s: USN日志删除删除成功！\n", __FUNCTION__);

    return true;
}

/*********************************************************
名称：   closeUSNFile
说明：   关闭USN存档
入参：   无
返回值： true  -- 关闭成功
        false -- 关闭失败
*********************************************************/
void
CUSNManager::closeUSNFile()
{
    if (INVALID_HANDLE_VALUE == m_hVol) {
        printf("%s: No need to close!\n", __FUNCTION__);
        return;
    }

    CloseHandle(m_hVol);
    m_hVol = INVALID_HANDLE_VALUE;
    memset(&m_UsnInfo, 0x00, sizeof(m_UsnInfo)); 

    return;
}

/*********************************************************
名称：   collectUSNRawData
说明：   获取并存储USN原始数据
入参：   pOutputPaht -- 导出的路径
返回值： true  -- 获取成功
        false -- 获取失败
*********************************************************/
bool
CUSNManager::collectUSNRawData()
{
    if (INVALID_HANDLE_VALUE == m_hVol) {
        printf("%s: Please open one USNFile First!",
            __FUNCTION__);
        return false;
    }

    char buffer[0x1000];
    DWORD BytesReturned;
    READ_USN_JOURNAL_DATA rujd = { 0, -1, 0, 0, 0, m_UsnInfo.UsnJournalID };
    for (; DeviceIoControl(m_hVol,
        FSCTL_READ_USN_JOURNAL,
        &rujd,
        sizeof(rujd),
        buffer,
        _countof(buffer),
        &BytesReturned,
        NULL);
        rujd.StartUsn = *(USN*)&buffer) {

        DWORD dwRetBytes = BytesReturned - sizeof(USN);
        PUSN_RECORD UsnRecord =
            (PUSN_RECORD)((PCHAR)buffer + sizeof(USN));

        if (dwRetBytes == 0) {
            return true;
        }

        while (dwRetBytes > 0) {
            MY_USN_RECORD myur = {
                UsnRecord->FileReferenceNumber
              , UsnRecord->ParentFileReferenceNumber
              , UsnRecord->TimeStamp
              , UsnRecord->Reason };

            memcpy(myur.FileName,
                   UsnRecord->FileName,
                   UsnRecord->FileNameLength);
            myur.FileName[UsnRecord->FileNameLength / 2]
                = L'\0';

            m_qData.push_back(myur);

            dwRetBytes -= UsnRecord->RecordLength;
            UsnRecord = (PUSN_RECORD)((PCHAR)UsnRecord + UsnRecord->RecordLength);
        }
    }

    return true;
}

/*********************************************************
名称：   dumpUSNInfo
说明：   导出USN文件操作信息
入参：   pOutputPaht -- 导出的路径
返回值： true  -- 导出成功
        false -- 导出失败
*********************************************************/
bool
CUSNManager::dumpUSNInfo(const char *pOutputPath)
{
    setlocale(LC_CTYPE, "chs");
    for (deque<MY_USN_RECORD>::const_iterator itor = m_qData.begin()
       ; itor != m_qData.end()
        ; ++itor) {

        const MY_USN_RECORD& mur = *itor;
        FILETIME timestamp;
        FileTimeToLocalFileTime(&(FILETIME&)mur.TimeStamp, &timestamp);
        SYSTEMTIME st;
        FileTimeToSystemTime(&timestamp, &st);
        printf("%04d-%02d-%02d %02d:%02d:%02d ", 
            st.wYear, st.wMonth, st.wDay, 
            st.wHour, st.wMinute, st.wSecond);

        if (mur.Reason&USN_REASON_DATA_OVERWRITE)
        {
            printf("%s|", "DATA_OVERWRITE");
        }
        if (mur.Reason&USN_REASON_DATA_EXTEND)
        {
            printf("%s|", "DATA_EXTEND");
        }
        if (mur.Reason&USN_REASON_DATA_TRUNCATION)
        {
            printf("%s|", "DATA_TRUNCATION");
        }
        if (mur.Reason&USN_REASON_NAMED_DATA_OVERWRITE)
        {
            printf("%s|", "NAMED_DATA_OVERWRITE");
        }
        if (mur.Reason&USN_REASON_NAMED_DATA_EXTEND)
        {
            printf("%s|", "NAMED_DATA_EXTEND");
        }
        if (mur.Reason&USN_REASON_NAMED_DATA_TRUNCATION)
        {
            printf("%s|", "NAMED_DATA_TRUNCATION");
        }
        if (mur.Reason&USN_REASON_FILE_CREATE)
        {
            printf("%s|", "FILE_CREATE");
        }
        if (mur.Reason&USN_REASON_FILE_DELETE)
        {
            printf("%s|", "FILE_DELETE");
        }
        if (mur.Reason&USN_REASON_EA_CHANGE)
        {
            printf("%s|", "EA_CHANGE");
        }
        if (mur.Reason&USN_REASON_SECURITY_CHANGE)
        {
            printf("%s|", "SECURITY_CHANGE");
        }
        if (mur.Reason&USN_REASON_RENAME_OLD_NAME)
        {
            printf("%s|", "RENAME_OLD_NAME");
        }
        if (mur.Reason&USN_REASON_RENAME_NEW_NAME)
        {
            printf("%s|", "RENAME_NEW_NAME");
        }
        if (mur.Reason&USN_REASON_INDEXABLE_CHANGE)
        {
            printf("%s|", "INDEXABLE_CHANGE");
        }
        if (mur.Reason&USN_REASON_BASIC_INFO_CHANGE)
        {
            printf("%s|", "BASIC_INFO_CHANGE");
        }
        if (mur.Reason&USN_REASON_HARD_LINK_CHANGE)
        {
            printf("%s|", "HARD_LINK_CHANGE");
        }
        if (mur.Reason&USN_REASON_COMPRESSION_CHANGE)
        {
            printf("%s|", "COMPRESSION_CHANGE");
        }
        if (mur.Reason&USN_REASON_ENCRYPTION_CHANGE)
        {
            printf("%s|", "ENCRYPTION_CHANGE");
        }
        if (mur.Reason&USN_REASON_OBJECT_ID_CHANGE)
        {
            printf("%s|", "OBJECT_ID_CHANGE");
        }
        if (mur.Reason&USN_REASON_REPARSE_POINT_CHANGE)
        {
            printf("%s|REPARSE_POINT_CHANGE", "");
        }
        if (mur.Reason&USN_REASON_STREAM_CHANGE)
        {
            printf("%s|", "STREAM_CHANGE");
        }
        if (mur.Reason&USN_REASON_TRANSACTED_CHANGE)
        {
            printf("%s|", "TRANSACTED_CHANGE");
        }
        if (mur.Reason&USN_REASON_CLOSE)
        {
            printf("%s|", "CLOSE");
        }

        printf("\n  ");
        PrintFullPath(mur, m_qData);
        printf("\n");
    }
    printf("\n");
    return true;
}

bool 
CUSNManager::PrintFullPath(
    const MY_USN_RECORD&        mur, 
    const deque<MY_USN_RECORD>& con
)
{
    if ((mur.FileReferenceNumber & 0x0000FFFFFFFFFFFF) == 5)
        return true;

    deque<MY_USN_RECORD>::const_iterator recent = con.end();
    for (deque<MY_USN_RECORD>::const_iterator itor = con.begin()
       ; itor != con.end() && itor->TimeStamp.QuadPart <= mur.TimeStamp.QuadPart
       ; ++itor) {
        if (itor->FileReferenceNumber == mur.ParentFileReferenceNumber)
            recent = itor;
    }
    // 它的父目录可能也已被删除，所以要先在记录集中找找  
    if (recent != con.end()) {
        bool r = PrintFullPath(*recent, con);
        printf("\\%S", mur.FileName);
        return r;
    }

    // 如果记录中没有，再去看看这个文件实际存在否  
    bool r = GetFullPathByFileReferenceNumber(m_hVol, mur.ParentFileReferenceNumber);
    if (r) {
        printf("\\%S", mur.FileName);
    }
    else {
        printf("???\\%S", mur.FileName);
    }

    return r;
}

bool 
CUSNManager::GetFullPathByFileReferenceNumber(
    HANDLE      hVol, 
    DWORDLONG   FileReferenceNumber
)
{
    if ((FileReferenceNumber & 0x0000FFFFFFFFFFFF) == 5) {
        return true;
    }

    bool ret = false;
    DWORD BytesReturned;
    NTFS_VOLUME_DATA_BUFFER nvdb;

    // 仅是事例，没有作优化 1.作为递归调用，这一步应当提取出来 
    // 2.如果多次调用，DirectoryFileReferenceNumber没必要被重复获取  
    if (DeviceIoControl(hVol, FSCTL_GET_NTFS_VOLUME_DATA, NULL, 0
        , &nvdb, sizeof(nvdb), &BytesReturned, NULL))
    {
        NTFS_FILE_RECORD_INPUT_BUFFER nfrib;
        nfrib.FileReferenceNumber.QuadPart = FileReferenceNumber;
        size_t len = sizeof(NTFS_FILE_RECORD_OUTPUT_BUFFER) + nvdb.BytesPerFileRecordSegment - 1;
        NTFS_FILE_RECORD_OUTPUT_BUFFER* nfrob = (PNTFS_FILE_RECORD_OUTPUT_BUFFER)operator new(len);
        if (DeviceIoControl(hVol, FSCTL_GET_NTFS_FILE_RECORD, &nfrib, sizeof(nfrib)
            , nfrob, len, &BytesReturned, NULL)) {

            // a 48-bit index and a 16-bit sequence number  
            if ((nfrib.FileReferenceNumber.QuadPart & 0x0000FFFFFFFFFFFF) == nfrob->FileReferenceNumber.QuadPart)
            {
                PFILE_RECORD_HEADER frh = (PFILE_RECORD_HEADER)nfrob->FileRecordBuffer;
                for (PATTRIBUTE attr = (PATTRIBUTE)((LPBYTE)frh + frh->AttributesOffset)
                   ; attr->AttributeType != -1
                   ; attr = (PATTRIBUTE)((LPBYTE)attr + attr->Length)) {
                    if (attr->AttributeType == AttributeFileName) {
                        PFILENAME_ATTRIBUTE name = (PFILENAME_ATTRIBUTE)((LPBYTE)attr + PRESIDENT_ATTRIBUTE(attr)->ValueOffset);

                        // long name  
                        if ((name->NameType & 1) == 1) {
                            if (GetFullPathByFileReferenceNumber(hVol, name->DirectoryFileReferenceNumber)) {
                                printf("\\%.*S", name->NameLength, name->Name);
                                ret = true;
                            }
                        }
                    }
                }
            }
        }
        operator delete(nfrob);
    }

    return ret;
}




