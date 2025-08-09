# 2025-08-10T01:09:30.890031100
import vitis

client = vitis.create_client()
client.set_workspace(path="tiny_sha3")

vitis.dispose()

