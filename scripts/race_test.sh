#!/bin/bash

echo "=========================================="
echo "   Client Race Condition Stress Test"
echo "=========================================="
echo "[!] Ensure your ./server is running in another terminal before proceeding!"
read -p "Press [Enter] when the server is ready..."

# 1. Create dummy C files directly in the main folder
echo "[*] Creating 5 dummy C files in the main folder..."
for i in {1..5}; do
cat <<EOF > test_race_$i.c
#include <stdio.h>
int main() {
    int sum = 0;
    for(int j=0; j<10000; j++) sum += j;
    return 0;
}
EOF
done

# 2. Launch concurrent clients
echo "[*] Launching 20 concurrent clients simultaneously..."
START_TIME=$(date +%s.%N)

for i in {1..20}; do
    # Assign User IDs from 1 to 20 (fits your 0-99 range)
    USER_ID=$i
    
    # Each client will compile 2 files
    FILE1="test_race_$(( (i % 5) + 1 )).c"
    FILE2="test_race_$(( ((i+1) % 5) + 1 )).c"

    # Pipe Role 2 (User) and User ID to the client, run in background (&)
    echo -e "2\n${USER_ID}\n" | ./client "$FILE1" "$FILE2" > /dev/null 2>&1 &
done

# 3. Wait for all clients to finish
echo "[*] All 20 clients dispatched. Waiting for network resolution..."
wait

END_TIME=$(date +%s.%N)
TOTAL_TIME=$(echo "$END_TIME - $START_TIME" | bc)
echo "[*] All clients finished executing in $TOTAL_TIME seconds."

# 4. Check Server Telemetry using an Admin account
echo -e "\n--- Validating Server Telemetry ---"
echo "We sent 20 clients * 2 files = 40 total compilation jobs."
echo "If your mutex locks are perfectly thread-safe, 'Total jobs' should exactly equal 40 and 'Active jobs' should be 0."
echo "-----------------------------------"

# Role 1 (Admin), Admin ID 100 (fits your 100-114 range), Opcode 2 (Check Status)
echo -e "1\n100\n2\n" | ./client

# 5. Cleanup the main folder
echo -e "\n[*] Cleaning up generated test files and object files..."
rm -f test_race_*.c test_race_*.o

echo "[*] Test Complete!"