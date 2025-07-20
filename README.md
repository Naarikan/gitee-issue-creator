# gitee-issue-creator

A CLI toolset for quickly creating issues on Gitee.

## Features

- Manage multiple repositories
- Token encryption and secure storage
- Create issues interactively or with command-line arguments
- Set default repository, add and delete repositories

## Prerequisites

Make sure you have these dependencies installed on your system:

- **C++17 compatible compiler**
- **CMake** (version 3.10 or higher, preferably 3.15+ for `cmake --install` support)
- **libcurl**
- **OpenSSL**
- **SQLite3**

You also need the following external libraries included in the project:

- [cxxopts](https://github.com/jarro2783/cxxopts) for command line argument parsing

## Installation

Clone the repository and build the project:

```bash
git clone https://github.com/Naarikan/gitee-issue-creator.git
cd gitee-issue-creator
mkdir build && cd build
cmake ..
```
If your CMake version is 3.15 or higher:
```bash
cmake --build .
sudo cmake --install .
```
If your CMake version is lower than 3.15:
```bash
make
sudo make install
```

The install() command in the CMakeLists.txt copies the executable to your system’s binary directory (usually /usr/local/bin), so you can run the tool globally without needing ./ prefix.

## Usage
Run the tool with gitee-issue from anywhere in your terminal.
To see available options:
```bash
gitee-issue --help
```

