#include "targetver.h"
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files:
#include <windows.h>

#include "resource.h"
#include "ClientWindow.h"
#include "Configs.h"
#include "ClientDriver.h"
#include "IClientDriver.h"
#include "NetworkStructs.h"
#include "Clipboard_Lite.h"

const char WindowClass[] = "RAT_PROJECT";

namespace SL {
    namespace RAT_Client {
        class ClientWindowImpl : public RAT::IClientDriver {
            RAT::ClientDriver ClientDriver_;
            Clipboard_Lite::Clipboard_Manager Clipboard_Manager_;

            std::shared_ptr<SL::WS_LITE::IWSocket> Socket_;

            HWND hWnd = nullptr;
            HWND ConnecthWnd = nullptr;
            HINSTANCE hInstance = nullptr;
            std::shared_ptr<SL::RAT::Client_Config> Config;
        public:

            static INT_PTR CALLBACK DlgProc(HWND hWndDlg, UINT Msg, WPARAM wParam, LPARAM lParam)
            {
                //static so it lasts for ever!!
                static char Hostname[256];
                switch (Msg)
                {
                case WM_INITDIALOG:
                    return TRUE;
                case WM_COMMAND:
                    switch (wParam)
                    {
                    case ID_TRYCONNTECT:
                        GetDlgItemTextA(hWndDlg, IDC_HOSTTEXTBOX, Hostname, sizeof(Hostname));
                        PostMessage(GetParent(hWndDlg), WM_COMMAND, ID_TRYCONNTECT, (LPARAM)Hostname);
                        return TRUE;
                    case ID_CANCELCONNTECT:
                        PostMessage(GetParent(hWndDlg), WM_COMMAND, ID_TRYCONNTECTCLOSING, 0);
                        EndDialog(hWndDlg, 0);
                        return TRUE;
                    }
                    break;
                case WM_CLOSE:
                case WM_DESTROY:
                    PostMessage(GetParent(hWndDlg), WM_COMMAND, ID_TRYCONNTECTCLOSING, 0);
                    EndDialog(hWndDlg, 0);
                    return TRUE;
                default:
                    return FALSE;
                }
                return FALSE;
            }

            LRESULT WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {

                switch (uMsg)
                {
                case WM_COMMAND:
                    switch (wParam)
                    {
                    case ID_TRYCONNTECT:
                        TryConnect(reinterpret_cast<const char*>(lParam));
                        break;
                    case IDD_CONNECTTODIALOG:
                        if (!IsWindow(ConnecthWnd))
                        {
                            ConnecthWnd = CreateDialog(hInstance, MAKEINTRESOURCE(IDD_CONNECTTO), hwnd, DlgProc);
                            ShowWindow(ConnecthWnd, SW_SHOW);
                        }
                        break;
                    case ID_TRYCONNTECTCLOSING:
                        ConnecthWnd = nullptr;
                        if (IsWindowVisible(hwnd) ==FALSE) {
                            PostQuitMessage(0);
                            break;
                        }
                        break;
                    }
                    break;
                case WM_PAINT:
                {
                    PAINTSTRUCT ps;
                    HDC hdc = BeginPaint(hwnd, &ps);
                    // TODO: Add any drawing code that uses hdc here...
                    EndPaint(hwnd, &ps);
                }
                break;
                case WM_DESTROY:
                    PostQuitMessage(0);
                    break;
                default:
                    return DefWindowProc(hwnd, uMsg, wParam, lParam);
                }
                return 0;


            }
            static LRESULT CALLBACK WindowProcStatic(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {

                if (uMsg == WM_NCCREATE)
                {
                    auto cs = (CREATESTRUCT*)lParam;
                    auto p = (ClientWindowImpl*)cs->lpCreateParams;
                    SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)p);
                    return TRUE;
                }
                else
                {
                    auto thisclass = (ClientWindowImpl*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
                    if (thisclass) {
                        return thisclass->WindowProc(hwnd, uMsg, wParam, lParam);
                    }
                }
                return DefWindowProc(hwnd, uMsg, wParam, lParam);
            }
            ClientWindowImpl(std::shared_ptr<SL::RAT::Client_Config>& config, const std::string& host) :Config(config), ClientDriver_(this) {

                hInstance = GetModuleHandle(NULL);
                WNDCLASSEXA wcex;
                wcex.cbSize = sizeof(wcex);

                wcex.style = CS_HREDRAW | CS_VREDRAW;
                wcex.lpfnWndProc = WindowProcStatic;
                wcex.cbClsExtra = 0;
                wcex.cbWndExtra = 0;
                wcex.hInstance = hInstance;
                wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_PROJECTICON));
                wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
                wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
                wcex.lpszMenuName = 0;
                wcex.lpszClassName = WindowClass;
                wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_PROJECTICON));

                RegisterClassExA(&wcex);
                hWnd = CreateWindowA(WindowClass, WindowTitle, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, this);
                if (hWnd)
                {
                    if (host.empty() || host.size() < 2) {
                        PostMessage(hWnd, WM_COMMAND ,IDD_CONNECTTODIALOG, 0);
                    }
                    else {
                        TryConnect(host.c_str());
                    }
                }
                Clipboard_Manager_ = Clipboard_Lite::CreateClipboard().onText([&](const std::string& text) {

                }).run();



            }
            virtual ~ClientWindowImpl() {
                Clipboard_Manager_.destroy();//make sure no race conditions occur
            }
            void Run() {

                MSG msg;
                BOOL bRet;
                while ((bRet = GetMessage(&msg, 0, 0, 0)) != 0)
                {
                    if (bRet == -1)
                    {
                        break;//get out of the loop
                    }
                    else
                    {
                        TranslateMessage(&msg);
                        DispatchMessage(&msg);
                    }
                }
            }
            void TryConnect(const char* hostname) {
                ClientDriver_.Connect(Config, hostname);
            }
            virtual void onConnection(const std::shared_ptr<SL::WS_LITE::IWSocket>& socket) override {
                Socket_ = socket;
                //make sure to show the window
                ShowWindow(hWnd, SW_SHOW);
                UpdateWindow(hWnd);
                std::cout <<"onConnection" << std::endl;
            }
            virtual void onMessage(const std::shared_ptr<SL::WS_LITE::IWSocket>& socket, const WS_LITE::WSMessage& msg) override {
                std::cout << "onMessage" << std::endl;
            }
            virtual void onDisconnection(const std::shared_ptr<SL::WS_LITE::IWSocket>& socket, unsigned short code, const std::string& msg) override {
                PostMessage(hWnd, WM_COMMAND, IDD_CONNECTTODIALOG, 0);
                ShowWindow(hWnd, SW_HIDE);
                UpdateWindow(hWnd);
                std::cout << msg << std::endl;
            }
            virtual void onMonitorsChanged(const std::vector<Screen_Capture::Monitor>& monitors) override {
                std::cout << "onMonitorsChanged" << std::endl;
            }
            virtual void onFrameChanged(const RAT::Image& img, const SL::Screen_Capture::Monitor& monitor) override {
                std::cout << "onFrameChanged" << std::endl;
                HDC hdc = GetDC(hWnd);

                BITMAPINFO info;
                ZeroMemory(&info, sizeof(BITMAPINFO));
                info.bmiHeader.biBitCount = 32;
                info.bmiHeader.biWidth = img.Rect_.Width;
                info.bmiHeader.biHeight = img.Rect_.Height;
                info.bmiHeader.biPlanes = 1;
                info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
                info.bmiHeader.biSizeImage = img.Length;
                info.bmiHeader.biCompression = BI_RGB;
                StretchDIBits(hdc, img.Rect_.left(), img.Rect_.top(), img.Rect_.Width, img.Rect_.Height, 0, 0, img.Rect_.Width, img.Rect_.Height, img.Data, &info, DIB_RGB_COLORS, SRCCOPY);
                ReleaseDC(hWnd, hdc);
            }
            virtual void onNewFrame(const RAT::Image& img, const SL::Screen_Capture::Monitor& monitor) override {
                std::cout << "onNewFrame" << std::endl;
                HDC hdc = GetDC(hWnd);

                BITMAPINFO info;
                ZeroMemory(&info, sizeof(BITMAPINFO));
                info.bmiHeader.biBitCount = 32;
                info.bmiHeader.biWidth = img.Rect_.Width;
                info.bmiHeader.biHeight = img.Rect_.Height;
                info.bmiHeader.biPlanes = 1;
                info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
                info.bmiHeader.biSizeImage = img.Length;
                info.bmiHeader.biCompression = BI_RGB;
                StretchDIBits(hdc, img.Rect_.left(), img.Rect_.top(), img.Rect_.Width, img.Rect_.Height, 0, 0, img.Rect_.Width, img.Rect_.Height, img.Data, &info, DIB_RGB_COLORS, SRCCOPY);
                ReleaseDC(hWnd, hdc);
            }
            virtual void onMouseImageChanged(const RAT::Image& img) override {
                std::cout << "onMouseImageChanged" << std::endl;
            }
            virtual void onMousePositionChanged(const RAT::Point& mevent) override {
                std::cout << "onMousePositionChanged" << std::endl;
            }
            virtual void onClipboardChanged(const std::string& text) override {
                std::cout << "onClipboardChanged" << std::endl;
            }

        };


        ClientWindow::ClientWindow(std::shared_ptr<SL::RAT::Client_Config>& config, const std::string& host) {
            ClientWindowImpl_ = new ClientWindowImpl(config, host);
        }
        ClientWindow::~ClientWindow() {
            delete ClientWindowImpl_;
        }

        void ClientWindow::Run() {
            ClientWindowImpl_->Run();
        }
    }
}