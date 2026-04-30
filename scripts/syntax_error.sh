#!/bin/bash
# Save as scripts/ipc_error_test.sh

echo "[*] Creating a C file with a deliberate syntax error..."
cat <<EOF > bad_code.c
#include <stdio.h>
int main() {
    printf("I am missing a semicolon")
    return 0;
}
EOF

echo "[*] Sending bad code to the server..."
# Role 2 (User), ID 50
echo -e "2\n50\n" | ./client bad_code.c

echo "[*] Checking cleanup..."
rm -f bad_code.c
echo "If your client printed the GCC syntax error, your IPC pipes work perfectly!"