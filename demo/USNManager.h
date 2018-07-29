#include <Windows.h>
#include <stdio.h>
#include <string>
#include <iostream>
#include <fstream> 
#include <winioctl.h>
#include <set>
#include <deque>

using namespace std;

class CUSNManager
{
public:
    CUSNManager();
    virtual ~CUSNManager();
    bool openUSNFile(const char *pDriverName);
    bool deleteUSNFile();
    void closeUSNFile();
    bool dumpUSNInfo(const char *pOutputPath);
    bool collectUSNRawData();

protected:
    struct MY_USN_RECORD
    {
        DWORDLONG FileReferenceNumber;
        DWORDLONG ParentFileReferenceNumber;
        LARGE_INTEGER TimeStamp;
        DWORD Reason;
        WCHAR FileName[MAX_PATH];
    };

    typedef struct {
        ULONG Type;
        USHORT UsaOffset;
        USHORT UsaCount;
        USN Usn;
    } NTFS_RECORD_HEADER, *PNTFS_RECORD_HEADER;

    typedef struct {
        NTFS_RECORD_HEADER Ntfs;
        USHORT SequenceNumber;
        USHORT LinkCount;
        USHORT AttributesOffset;
        USHORT Flags;               // 0x0001 = InUse, 0x0002 = Directory  
        ULONG BytesInUse;
        ULONG BytesAllocated;
        ULONGLONG BaseFileRecord;
        USHORT NextAttributeNumber;
    } FILE_RECORD_HEADER, *PFILE_RECORD_HEADER;

    typedef enum {
        AttributeStandardInformation = 0x10,
        AttributeAttributeList = 0x20,
        AttributeFileName = 0x30,
        AttributeObjectId = 0x40,
        AttributeSecurityDescriptor = 0x50,
        AttributeVolumeName = 0x60,
        AttributeVolumeInformation = 0x70,
        AttributeData = 0x80,
        AttributeIndexRoot = 0x90,
        AttributeIndexAllocation = 0xA0,
        AttributeBitmap = 0xB0,
        AttributeReparsePoint = 0xC0,
        AttributeEAInformation = 0xD0,
        AttributeEA = 0xE0,
        AttributePropertySet = 0xF0,
        AttributeLoggedUtilityStream = 0x100
    } ATTRIBUTE_TYPE, *PATTRIBUTE_TYPE;

    typedef struct {
        ATTRIBUTE_TYPE AttributeType;
        ULONG Length;
        BOOLEAN Nonresident;
        UCHAR NameLength;
        USHORT NameOffset;
        USHORT Flags;               // 0x0001 = Compressed  
        USHORT AttributeNumber;
    } ATTRIBUTE, *PATTRIBUTE;

    typedef struct {
        ATTRIBUTE Attribute;
        ULONGLONG LowVcn;
        ULONGLONG HighVcn;
        USHORT RunArrayOffset;
        UCHAR CompressionUnit;
        UCHAR AlignmentOrReserved[5];
        ULONGLONG AllocatedSize;
        ULONGLONG DataSize;
        ULONGLONG InitializedSize;
        ULONGLONG CompressedSize;    // Only when compressed  
    } NONRESIDENT_ATTRIBUTE, *PNONRESIDENT_ATTRIBUTE;

    typedef struct {
        ATTRIBUTE Attribute;
        ULONG ValueLength;
        USHORT ValueOffset;
        USHORT Flags;               // 0x0001 = Indexed  
    } RESIDENT_ATTRIBUTE, *PRESIDENT_ATTRIBUTE;

    typedef struct {
        ULONGLONG CreationTime;
        ULONGLONG ChangeTime;
        ULONGLONG LastWriteTime;
        ULONGLONG LastAccessTime;
        ULONG FileAttributes;
        ULONG AlignmentOrReservedOrUnknown[3];
        ULONG QuotaId;                        // NTFS 3.0 only  
        ULONG SecurityId;                     // NTFS 3.0 only  
        ULONGLONG QuotaCharge;                // NTFS 3.0 only  
        USN Usn;                              // NTFS 3.0 only  
    } STANDARD_INFORMATION, *PSTANDARD_INFORMATION;

    typedef struct {
        ULONGLONG DirectoryFileReferenceNumber;
        ULONGLONG CreationTime;   // Saved when filename last changed  
        ULONGLONG ChangeTime;     // ditto  
        ULONGLONG LastWriteTime;  // ditto  
        ULONGLONG LastAccessTime; // ditto  
        ULONGLONG AllocatedSize;  // ditto  
        ULONGLONG DataSize;       // ditto  
        ULONG FileAttributes;     // ditto  
        ULONG AlignmentOrReserved;
        UCHAR NameLength;
        UCHAR NameType;           // 0x01 = Long, 0x02 = Short  
        WCHAR Name[1];
    } FILENAME_ATTRIBUTE, *PFILENAME_ATTRIBUTE;

    bool getUSNBasicInfo(HANDLE hVol);
    bool isUSNSupported(const char *pDriverName);
    HANDLE getDriverHandle(const char *pDriverName);
    bool GetFullPathByFileReferenceNumber(HANDLE hVol, DWORDLONG FileReferenceNumber);
    bool PrintFullPath(const MY_USN_RECORD& mur, const deque<MY_USN_RECORD>& con);

private:
    USN_JOURNAL_DATA            m_UsnInfo;
    HANDLE                      m_hVol;
    deque<MY_USN_RECORD>        m_qData;
};
