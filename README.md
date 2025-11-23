# Simple CPP server for UDP/TCP

## Building

Update and upgrade the system.

```sudo apt-get update & sudo apt-get upgrade -y```

Install dependencies.

```sudo apt-get install make git -y & sudo apt install gcc-14 g++-14 -y & sudo snap install cmake --classic & sudo apt autoremove```  

Clone git repository.

```git clone https://github.com/KRISgardian/testovoe.git --depth 1 & cd testovoe```

Configure cmake project.

```mkdir build & cmake -G "Unix Makefiles" -B build```  

Build project.

```cmake --build build```

Install project.

```cmake --install build```

## Running

### Running as daemon

Copy systemd configuration file to system directory.

```cp testovoe.service /etc/systemd/system/testovoe.service```  

Enable configuration file.

```systemctl enable testovoe```

### Running as simple binary.

Run the executable.

```cd bin & ./testovoe```

or simply

```./bin/testovoe```


## CI/CD

[![CI Status](https://github.com/KRISgardian/testovoe/actions/workflows/build.yml/badge.svg)](https://github.com/KRISgardian/testovoe/actions)