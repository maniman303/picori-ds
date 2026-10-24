# shellcheck shell=sh
# Shared SSH transport; source from the build/deploy helpers, do not execute.
CONFIG=${DEPLOY_CONFIG:-"$ROOT/scripts/deploy-rgdsplus.conf"}
[ -f "$CONFIG" ] || { echo "Missing $CONFIG; copy deploy-rgdsplus.conf.example first" >&2; exit 2; }
chmod 600 "$CONFIG"
# shellcheck disable=SC1090
. "$CONFIG"
: "${DEVICE_IP:?Set DEVICE_IP in the device config}"
DEVICE_USER=${DEVICE_USER:-root}
case "$DEVICE_IP" in ''|*[!0-9.]*|.*|*.) echo 'DEVICE_IP must be an IPv4 address' >&2; exit 2;; esac
case "$DEVICE_USER" in ''|*[!a-zA-Z0-9_-]*) echo 'Invalid DEVICE_USER' >&2; exit 2;; esac
for tool in ssh scp; do
    command -v "$tool" >/dev/null 2>&1 || { echo "Missing tool: $tool" >&2; exit 2; }
done
if [ -n "${DEVICE_PASSWORD:-}" ]; then
    command -v sshpass >/dev/null 2>&1 || { echo 'Missing tool: sshpass' >&2; exit 2; }
    SSHPASS=$DEVICE_PASSWORD
    export SSHPASS
fi
ssh_device() {
    if [ -n "${DEVICE_PASSWORD:-}" ]; then
        sshpass -e ssh -o StrictHostKeyChecking=accept-new -o ConnectTimeout=10 "$DEVICE_USER@$DEVICE_IP" "$@"
    else
        ssh -o BatchMode=yes -o StrictHostKeyChecking=accept-new -o ConnectTimeout=10 "$DEVICE_USER@$DEVICE_IP" "$@"
    fi
}
scp_device() {
    if [ -n "${DEVICE_PASSWORD:-}" ]; then
        sshpass -e scp -O -C -o StrictHostKeyChecking=accept-new -o ConnectTimeout=10 "$@"
    else
        scp -O -C -o BatchMode=yes -o StrictHostKeyChecking=accept-new -o ConnectTimeout=10 "$@"
    fi
}
