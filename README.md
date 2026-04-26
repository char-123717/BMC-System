
# Mini BMC — Baseboard Management Controller Simulator

A compact, portable BMC-like simulator implemented for Linux (WSL) using C++ and small helper scripts. The project exposes a simple TCP socket command interface (port 8888 by default) to query basic system sensors: CPU, memory, temperature, disk, and uptime. Responses are plain JSON strings so they are easy to parse.

## Contents

- `src/` — C++ server implementation (example included below)
- `scripts/` — small shell helpers used by the server
- `bmc_server` — compiled binary (output of build)

## Requirements

- Windows 10/11 with WSL (Ubuntu recommended) for Windows users
- Linux with `g++` and `netcat` (`nc`) installed
- POSIX tools: `bash`, `awk`, `grep`, `df`, `free`, `uptime`

## 1. Environment (Windows + WSL)

1. Install WSL (PowerShell as Admin):

```powershell
wsl --install
```

2. After install, reboot if prompted. Then list installed distributions:

```bash
wsl -l -v
```

3. Enter your Linux environment:

```bash
wsl
```

## 2. Project setup

From inside WSL or on a native Linux shell run:

```bash
cd /mnt/d
mkdir -p Project/BMC
cd Project/BMC
mkdir -p src scripts
```

Place the example server source in `src/server.cpp` (example below) and the helper scripts in `scripts/`.

## 3. Helper scripts

Create the following helper scripts under `scripts/`. On some Windows-mounted filesystems the execute bit can't be set — the server calls these via `bash` so `chmod` isn't required. These are the exact script contents used by the server.

`scripts/get_cpu.sh`

```bash
#!/bin/bash

CPU=$(grep 'cpu ' /proc/stat | awk '{usage=($2+$4)*100/($2+$4+$5)} END {print usage}')

echo $CPU
```

`scripts/get_mem.sh`

```bash
#!/bin/bash
# Percentage used memory
free | awk '/Mem:/ {printf "%.2f", $3/$2*100}'
```

`scripts/get_temp.sh`

```bash
#!/bin/bash
# Robust temperature probe:
# 1) /sys/class/thermal/thermal_zone*/temp
# 2) /sys/class/hwmon/hwmon*/temp*_input
# 3) `sensors` output (if lm-sensors installed)
# If none available, print N/A

for f in /sys/class/thermal/thermal_zone*/temp; do
	if [ -r "$f" ]; then
		val=$(cat "$f" 2>/dev/null)
		if [ -n "$val" ]; then
			# assume millidegrees
			awk -v v="$val" 'BEGIN{printf "%.1f", v/1000}'
			exit 0
		fi
	fi
done

for f in /sys/class/hwmon/hwmon*/temp*_input; do
	if [ -r "$f" ]; then
		val=$(cat "$f" 2>/dev/null)
		if [ -n "$val" ]; then
			awk -v v="$val" 'BEGIN{printf "%.1f", v/1000}'
			exit 0
		fi
	fi
done

if command -v sensors >/dev/null 2>&1; then
	out=$(sensors 2>/dev/null | awk '/^Package id 0:|^Core 0:|^temp1:/ {gsub(/\+|°C/,"",$2); print $2; exit}')
	if [ -n "$out" ]; then
		echo "$out"
		exit 0
	fi
fi

echo "N/A"
```

`scripts/get_disk.sh`

```bash
#!/bin/bash
# Root filesystem usage
df -h / | awk 'NR==2{print $5}'
```

`scripts/get_uptime.sh`

```bash
#!/bin/bash
uptime -p
```



## 4. Build

Compile with `g++` (C++11/17 compatible). Example:

```bash
g++ src/server.cpp -o bmc_server -std=c++17
```

If you need to enable additional features (e.g., multi-threading), add `-pthread` and update the source accordingly.

## 5. Run

Start the server:

```bash
./bmc_server
```

You should see:

```
🔥 BMC Mini Server running on port 8888...
```

Open another terminal and connect with `nc`:

```bash
nc localhost 8888
```

Type commands (one per line): `GET_CPU`, `GET_MEM`, `GET_TEMP`, `GET_DISK`, `GET_UPTIME`, or `QUIT` to close.

## 6. Example interactions

Client: `GET_CPU`

Server:

```json
{"cmd":"GET_CPU","value":12.34,"unit":"%"}
```

Client: `GET_UPTIME`

Server:

```json
{"cmd":"GET_UPTIME","value":"up 2 hours"}
```

## 7. Architecture

Client (nc/telnet)
  ↓
TCP Socket Server (`src/server.cpp`)
  ↓
Command router (simple string-based dispatcher)
  ↓
System sensors (scripts, /proc, df, uptime)
  ↓
JSON-style responses

## 8. Features

- TCP socket server listening on port 8888
- Line-oriented command router (supports multi-command session)
- Linux integration via scripts and `/proc` for lightweight sensor sampling
- JSON-style responses for easy parsing

## 9. Future improvements

- Multi-threaded or asynchronous server for concurrent clients
- Proper JSON library (nlohmann/json) and strict typing
- Add authentication / access control
- Logging and audit trails
- systemd service file for auto-start on boot
- Redfish-like REST API wrapper