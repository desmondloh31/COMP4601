# 2025-08-09T23:42:24.927374800
import vitis

client = vitis.create_client()
client.set_workspace(path="tiny_sha3")

vitis.dispose()

