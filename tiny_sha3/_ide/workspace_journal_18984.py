# 2025-08-09T23:46:33.495768500
import vitis

client = vitis.create_client()
client.set_workspace(path="tiny_sha3")

comp = client.get_component(name="hls_component")
comp.run(operation="IMPLEMENTATION")

vitis.dispose()

