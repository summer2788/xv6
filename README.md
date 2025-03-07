

# Extended xv6 Operating System

## Introduction
This project extends the xv6 operating system with four key features: custom system calls, CPU scheduling enhancements, memory mapping (`mmap`), and page replacement strategies. These enhancements aim to improve the xv6 kernel's functionality and performance, offering a more sophisticated environment for educational purposes.

## Table of Contents
- [Installation](#installation)
- [Features](#features)
  - [System Calls](#system-calls)
  - [CPU Scheduling](#cpu-scheduling)
  - [Memory Mapping (Mmap)](#memory-mapping-mmap)
  - [Page Replacement](#page-replacement)
- [Usage](#usage)
- [Building and Running](#building-and-running)
- [Contributors](#contributors)
- [License](#license)

## Installation
Before you start, ensure you have a development environment set up for xv6. This typically requires:
- GCC
- QEMU
- Make
- Git (for version control)

Clone the modified xv6 repository:
```bash
git clone [https://github.com/summer2788/xv6.git] xv6-extended
cd xv6-extended
```

## Features

### System Calls
We've added new system calls to the xv6 kernel to support advanced operations. These include:
- `sys_myCall`: An example system call (replace with your actual system calls and describe their functionalities).

### CPU Scheduling
Implemented a new CPU scheduling algorithm to improve process management and CPU utilization. This includes:
- Round Robin: Enhanced the existing scheduling mechanism to ensure fairness and efficiency.
- Priority Scheduling: Processes are now scheduled based on priority, with mechanisms to prevent starvation.

### Memory Mapping (Mmap)
Introduced memory mapping functionality, allowing processes to map files into their address space. This enhances file I/O operations by leveraging direct memory access.

### Page Replacement
Implemented advanced page replacement strategies to optimize memory usage and reduce page faults. These strategies include:
- Least Recently Used (LRU): Pages that have not been accessed recently are replaced first.
- Clock Algorithm: A circular queue design that approximates LRU with reduced overhead.

## Usage
To leverage the new features in your xv6 projects, refer to the following usage examples:
- System Calls: `int myCall(void)`
- CPU Scheduling: Managed automatically by the kernel. Priority for processes can be adjusted using the `setpriority` system call.
- Memory Mapping: Use the `mmap` system call to map files into a process's memory space.
- Page Replacement: The kernel manages page replacement automatically based on the configured strategy.

## Building and Running
To build and run the extended xv6:
```bash
make clean
make
make qemu-nox
```

## Contributors
- [Jongeun Park](summer2788@g.skku.edu)

## License
This project inherits the license from the original xv6 project, which is based on the MIT License.

