#include "FilePicker.hpp"

#include <windows.h>
#include <shobjidl.h>
#include <string>


namespace FilePicker {
    std::string openFile() {
        std::string result;


        CoInitializeEx(
            nullptr,
            COINIT_APARTMENTTHREADED
        );


        IFileOpenDialog *dialog = nullptr;


        HRESULT hr = CoCreateInstance(
            CLSID_FileOpenDialog,
            nullptr,
            CLSCTX_INPROC_SERVER,
            IID_PPV_ARGS(&dialog)
        );


        if (SUCCEEDED(hr)) {
            if (SUCCEEDED(dialog->Show(nullptr))) {
                IShellItem *item;


                if (SUCCEEDED(dialog->GetResult(&item))) {
                    PWSTR path;


                    if (SUCCEEDED(
                        item->GetDisplayName(
                            SIGDN_FILESYSPATH,
                            &path
                        ))) {
                        char buffer[MAX_PATH];

                        WideCharToMultiByte(
                            CP_UTF8,
                            0,
                            path,
                            -1,
                            buffer,
                            MAX_PATH,
                            nullptr,
                            nullptr
                        );


                        result = buffer;


                        CoTaskMemFree(path);
                    }


                    item->Release();
                }
            }


            dialog->Release();
        }


        CoUninitialize();


        return result;
    }


    std::string openFolder() {
        std::string result;


        CoInitializeEx(
            nullptr,
            COINIT_APARTMENTTHREADED
        );


        IFileOpenDialog *dialog = nullptr;


        HRESULT hr = CoCreateInstance(
            CLSID_FileOpenDialog,
            nullptr,
            CLSCTX_INPROC_SERVER,
            IID_PPV_ARGS(&dialog)
        );


        if (SUCCEEDED(hr)) {
            DWORD options;


            dialog->GetOptions(&options);


            dialog->SetOptions(
                options | FOS_PICKFOLDERS
            );


            if (SUCCEEDED(dialog->Show(nullptr))) {
                IShellItem *item;


                if (SUCCEEDED(dialog->GetResult(&item))) {
                    PWSTR path;


                    if (SUCCEEDED(
                        item->GetDisplayName(
                            SIGDN_FILESYSPATH,
                            &path
                        ))) {
                        char buffer[MAX_PATH];


                        WideCharToMultiByte(
                            CP_UTF8,
                            0,
                            path,
                            -1,
                            buffer,
                            MAX_PATH,
                            nullptr,
                            nullptr
                        );


                        result = buffer;


                        CoTaskMemFree(path);
                    }


                    item->Release();
                }
            }


            dialog->Release();
        }


        CoUninitialize();


        return result;
    }


    std::string saveFile() {
        // We will implement when adding Excel output

        return "";
    }
}