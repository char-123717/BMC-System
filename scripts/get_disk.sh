#!/bin/bash
# Root filesystem usage
df -h / | awk 'NR==2{print $5}'
