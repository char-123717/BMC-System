#!/bin/bash
# Percentage used memory
free | awk '/Mem:/ {printf "%.2f", $3/$2*100}'
