@echo off
set PATH=%PATH%;c:\debug_symbols

rem cmd.exe
rem pprof --text c:\debug_symbols\test.exe heap_profile_.0325.heap
rem perl pprof --callgrind --alloc_objects c:\debug_symbols\test.exe heap_profile_.0325.heap> callgrind.heap_profiler_data
rem perl pprof --callgrind --show_bytes c:\debug_symbols\test.exe heap_profile_.0325.heap> callgrind.heap_profiler_data
perl pprof --callgrind c:\debug_symbols\test.exe heap_profile_.0325.heap> callgrind.heap_profiler_data

pause
