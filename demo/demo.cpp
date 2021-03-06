// demo.cpp: 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "USNManager.h"

#pragma comment(lib,"USNManager.lib")

int main()
{
    CUSNManager usnManager;

    const char *pPath = "D:\\";
    if (!usnManager.openUSNFile(pPath)) {
        return 0;
    }


    //usnManager.deleteUSNFile();
    usnManager.collectUSNRawData();

    usnManager.dumpUSNInfo("");

    usnManager.closeUSNFile();

    system("pause");
    return 0;
}

