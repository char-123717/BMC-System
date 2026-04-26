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
