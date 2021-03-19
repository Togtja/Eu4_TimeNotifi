#include <Windows.h>

int ReadMemoryInt(HANDLE processHandle, LPCVOID address) {
    int buffer = 0;
    SIZE_T NumberOfBytesToRead = sizeof(buffer); //this is equal to 4
    SIZE_T NumberOfBytesActuallyRead;
    BOOL err = ReadProcessMemory(processHandle, address, &buffer, NumberOfBytesToRead, &NumberOfBytesActuallyRead);
    if (err || NumberOfBytesActuallyRead != NumberOfBytesToRead)
      /*an error occured*/ ;
    return buffer; 
}

int main(){


}