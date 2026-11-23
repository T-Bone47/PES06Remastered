import ctypes
from ctypes import wintypes

user32 = ctypes.windll.user32

def enum_windows_callback(hwnd, extra):
    pid = wintypes.DWORD()
    user32.GetWindowThreadProcessId(hwnd, ctypes.byref(pid))
    if pid.value == 4396:
        length = user32.GetWindowTextLengthW(hwnd)
        title = ctypes.create_unicode_buffer(length + 1)
        user32.GetWindowTextW(hwnd, title, length + 1)
        class_name = ctypes.create_unicode_buffer(256)
        user32.GetClassNameW(hwnd, class_name, 256)
        visible = user32.IsWindowVisible(hwnd)
        print(f'HWND: {hex(hwnd)}, Title: "{title.value}", Class: "{class_name.value}", Visible: {visible}')
    return True

WNDENUMPROC = ctypes.WINFUNCTYPE(ctypes.c_bool, wintypes.HWND, wintypes.LPARAM)
user32.EnumWindows(WNDENUMPROC(enum_windows_callback), 0)
