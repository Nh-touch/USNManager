#include "stdafx.h"
#include "USNManager.h"

/*********************************************************
���ƣ�   CUSNManager
˵����   ���캯��
��Σ�   
����ֵ�� 
*********************************************************/
CUSNManager::CUSNManager()
: m_hVol(INVALID_HANDLE_VALUE)
{

}

/*********************************************************
���ƣ�   CUSNManager
˵����   ��������
��Σ�
����ֵ��
*********************************************************/
CUSNManager::~CUSNManager()
{
    closeUSNFile(); 
}

/*********************************************************
���ƣ�   CUSNManager
˵����   �ж������̸�ʽ�Ƿ�֧��USN
��Σ�   pPath -- USN�ļ����ڴ��̺�(C,D,E,F...��
����ֵ�� bool -- true   ֧��
                false ��֧��
*********************************************************/
bool
CUSNManager::isUSNSupported(const char *pDriverName)
{
    printf("%s: �ж�������%s�Ƿ�NTFS��ʽ\n",
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
        sysNameBuf,             // �����̵�ϵͳ����FAT/NTFS��
        MAX_PATH);

    if (!status) {
        printf("%s: ��ȡ��Ϣʧ�ܣ�", __FUNCTION__);
        return false;
    }

    if (0 != strcmp(sysNameBuf, "NTFS")) {
        printf("%s: ��֧��USN �ļ�ϵͳ��: %s\n",
            __FUNCTION__, sysNameBuf);
        return false;
    }

    return true;
}

/*********************************************************
���ƣ�   getDriverHandle
˵����   ��ȡ�����̵ľ��
��Σ�   pPath -- USN�ļ����ڴ��̺�(C,D,E,F...��
����ֵ�� HANDLE -- ���
*********************************************************/
HANDLE
CUSNManager::getDriverHandle(const char *pDriverName)
{
    if (!isUSNSupported(pDriverName)) {
        return INVALID_HANDLE_VALUE;
    }

    HANDLE hVol = INVALID_HANDLE_VALUE;
    printf("%s: ��ȡ������%s ������\n",
        __FUNCTION__, pDriverName);
    char fileName[MAX_PATH] = { 0 };

    strcpy_s(fileName, "\\\\.\\");
    strcat_s(fileName, pDriverName);

    // Ϊ�˷������������תΪstring����ȥβ  
    string fileNameStr = (string)fileName;
    fileNameStr.erase(fileNameStr.find_last_of(":") + 1);

    printf("�����̵�ַ: %s", fileNameStr.data());

    // ��ȡ��� ���øú�����Ҫ����ԱȨ��
    hVol = CreateFileA(fileNameStr.data(),
        GENERIC_READ | GENERIC_WRITE,           // ����Ϊ0  
        FILE_SHARE_READ | FILE_SHARE_WRITE,     // ���������FILE_SHARE_WRITE  
        NULL, 
        OPEN_EXISTING,                          // �������OPEN_EXISTING, CREATE_ALWAYS���ܻᵼ�´���  
        FILE_ATTRIBUTE_READONLY,                // FILE_ATTRIBUTE_NORMAL���ܻᵼ�´���  
        NULL); 

    return hVol;
}

/*********************************************************
���ƣ�   getUSNBasicInfo
˵����   ��ȡUSN������Ϣ
��Σ�   hVol -- USN��־�ļ����
����ֵ�� true  -- ��ȡ�ɹ�
        false -- ��ȡʧ��
*********************************************************/
bool
CUSNManager::getUSNBasicInfo(HANDLE hVol)
{
    printf("%s: ��ȡUSN������Ϣ�����ں�������\n", 
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
        printf("%s: ��ȡUSN������Ϣʧ��!\n", __FUNCTION__);
        return false;
    }

    return true;
}

/*********************************************************
���ƣ�   openUSNFile
˵����   ��USN��־
��Σ�   pDriverName -- ����������
����ֵ�� true  -- ��־�򿪳ɹ�
        false -- ��־��ʧ��
*********************************************************/
bool
CUSNManager::openUSNFile(const char *pDriverName)
{
    // ��ȡ���
    m_hVol = getDriverHandle(pDriverName);
    if (INVALID_HANDLE_VALUE == m_hVol) {
        printf("%s: ��ȡ���ʧ��!\n", __FUNCTION__);
        return false;
    }

    // ��ʼ����־
    bool status = false;
    DWORD br;
    CREATE_USN_JOURNAL_DATA cujd;
    cujd.MaximumSize = 0;           // 0��ʾʹ��Ĭ��ֵ  
    cujd.AllocationDelta = 0;       // 0��ʾʹ��Ĭ��ֵ  
    status = DeviceIoControl(m_hVol,
        FSCTL_CREATE_USN_JOURNAL,
        &cujd,
        sizeof(cujd),
        NULL,
        0,
        &br,
        NULL);

    if (!status) {
        printf("%s: ��ʼ��USN��־ʧ��!\n", __FUNCTION__);
        return false;
    }

    // �洢������Ϣ
    return getUSNBasicInfo(m_hVol);
}

/*********************************************************
���ƣ�   deleteUSNFile
˵����   ɾ��USN�浵
��Σ�   ��
����ֵ�� true  -- ɾ���ɹ�
        false -- ɾ��ʧ��
*********************************************************/
bool
CUSNManager::deleteUSNFile()
{
    if (INVALID_HANDLE_VALUE == m_hVol) {
        printf("%s: Please open one USNFile First!", 
            __FUNCTION__);
        return false;
    }

    printf("%s: ɾ��USN��־!\n", __FUNCTION__);

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
        printf("%s: USN��־ɾ��ʧ�ܣ�\n", __FUNCTION__);
        return false;
    }

    printf("%s: USN��־ɾ��ɾ���ɹ���\n", __FUNCTION__);

    return true;
}

/*********************************************************
���ƣ�   closeUSNFile
˵����   �ر�USN�浵
��Σ�   ��
����ֵ�� true  -- �رճɹ�
        false -- �ر�ʧ��
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
���ƣ�   collectUSNRawData
˵����   ��ȡ���洢USNԭʼ����
��Σ�   pOutputPaht -- ������·��
����ֵ�� true  -- ��ȡ�ɹ�
        false -- ��ȡʧ��
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
���ƣ�   dumpUSNInfo
˵����   ����USN�ļ�������Ϣ
��Σ�   pOutputPaht -- ������·��
����ֵ�� true  -- �����ɹ�
        false -- ����ʧ��
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
    // ���ĸ�Ŀ¼����Ҳ�ѱ�ɾ��������Ҫ���ڼ�¼��������  
    if (recent != con.end()) {
        bool r = PrintFullPath(*recent, con);
        printf("\\%S", mur.FileName);
        return r;
    }

    // �����¼��û�У���ȥ��������ļ�ʵ�ʴ��ڷ�  
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

    // ����������û�����Ż� 1.��Ϊ�ݹ���ã���һ��Ӧ����ȡ���� 
    // 2.�����ε��ã�DirectoryFileReferenceNumberû��Ҫ���ظ���ȡ  
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




