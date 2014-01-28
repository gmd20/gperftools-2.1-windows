#include <iostream>

#include <stdlib.h>
#include <Windows.h>

using namespace std;

enum {
	kSharedMemorySize = 1024 * 1024 * 2  // 2 MB
};


enum OperationType {
	kCreateFile = 1,
	kWriteFile  = 2,
	kCloseFile  = 3
};

struct Operation {
	OperationType type;	
	HANDLE file_handle;
	size_t data_legnth;
	char filename_or_data[256];
};

void perform_file_operations(void * shared_memory)
{
	Operation *op = (Operation *)shared_memory;

	if (op->type == kWriteFile) {
		const char* buf = op->filename_or_data;
		size_t len = op->data_legnth;
		
		std::cout << " WriteFile: len=" << len << std::endl;

		while (len > 0) {
			DWORD wrote;
			BOOL ok = WriteFile(op->file_handle, buf, len, &wrote, NULL);
			// We do not use an asynchronous file handle, so ok==false means an error
			if (!ok) break;
			buf += wrote;
			len -= wrote;
		}
	} else if (op->type == kCreateFile) {
		const char * filename = op->filename_or_data;
		HANDLE fd = CreateFileA(filename, GENERIC_WRITE, 0, NULL,
			CREATE_ALWAYS, 0, NULL);
		if (fd != INVALID_HANDLE_VALUE && GetLastError() == ERROR_ALREADY_EXISTS)
			SetEndOfFile(fd);    // truncate the existing file

		cout << "CreateFileA filename=" << filename << " fd=" << fd << std::endl;
		op->file_handle = fd;
	}
	else if (op->type == kCloseFile) {
		CloseHandle(op->file_handle);
		cout << "CloseFile  fd=" << op->file_handle << std::endl;
	}
}

void run_tcmalloc_heap_profiler_collector() {
	// 1. create the shared memory 
	HANDLE  tcmalloc_shared_memory = CreateFileMapping(
		INVALID_HANDLE_VALUE,
		NULL,
		PAGE_READWRITE | SEC_COMMIT,
		0,
		kSharedMemorySize,
		"tcmalloc_shared_memory"
		);
	if (tcmalloc_shared_memory == NULL) {
		std::cout << "failed to CreateFileMapping tcmalloc_shared_memory!\n";
		exit(1);
	}

	LPVOID tcmalloc_shared_memory_address = MapViewOfFile(
		tcmalloc_shared_memory,
		FILE_MAP_ALL_ACCESS,
		0,
		0,
		kSharedMemorySize
		);

	if (tcmalloc_shared_memory_address == NULL) {
		std::cout << "failed to MapViewOfFile tcmalloc_shared_memory!\n";
		exit(1);
	}

	memset(tcmalloc_shared_memory_address, 0, kSharedMemorySize);


	// 2. mutex to protect teh shared memory
	HANDLE  tcmalloc_shared_memory_mutex = CreateMutex(
		NULL,
		false,
		"tcmalloc_shared_memory_mutex"
		);
	if (tcmalloc_shared_memory_mutex == NULL) {
		std::cout << "failed to CreateMutex tcmalloc_shared_memory_mutex!\n";
		exit(1);
	}

	// 3. event to signal the collector to read the data from shared memroy
	HANDLE  tcmalloc_file_op_event = CreateEvent(
		NULL,
		false,
		false,
		"tcmalloc_file_op_event"
		);
	if (tcmalloc_file_op_event == NULL) {
		std::cout << "failed to CreateEvent tcmalloc_file_op_event!\n";
		exit(1);
	}

	// 4.  event to signal the tcmalloc process that the shared memroy data has  been written into disk
	HANDLE  tcmalloc_file_op_finished_event = CreateEvent(
		NULL,
		false,
		false,
		"tcmalloc_file_op_finished_event"
		);
	if (tcmalloc_file_op_finished_event == NULL) {
		std::cout << "failed to CreateEvent tcmalloc_file_op_finished_event!\n";
		exit(1);
	}


	// 5. waiting for request from tcmalloc processs
	while (true) {
		if (WAIT_OBJECT_0 == WaitForSingleObject(tcmalloc_file_op_event, INFINITE)) {
			//6. wirte the data in the shared memroy to file.
			perform_file_operations(tcmalloc_shared_memory_address);

			// 7. resume the tcmalloc process
			SetEvent(tcmalloc_file_op_finished_event);
		}
		else {
			std::cout << "failed to WaitForSingleObject tcmalloc_shared_memory_mutex!\n";
		}
	}

	// 8. release all resource
	if (!UnmapViewOfFile(tcmalloc_shared_memory_address)) {
		std::cout << "failed to UnmapViewOfFile tcmalloc_shared_memory_address!\n";
	}

	if (!CloseHandle(tcmalloc_file_op_event)) {
		std::cout << "failed to CloseHandle tcmalloc_file_op_event!\n";
		exit(1);
	}

	if (!CloseHandle(tcmalloc_file_op_finished_event)) {
		std::cout << "failed to CloseHandle tcmalloc_file_op_finished_event!\n";
		exit(1);
	}

	if (!CloseHandle(tcmalloc_shared_memory_mutex)) {
		std::cout << "failed to CloseHandle tcmalloc_shared_memory_mutex!\n";
		exit(1);
	}

}
int main(int, char**)
{
	run_tcmalloc_heap_profiler_collector();
	return 0;
}






