# 2025-08-08T15:46:08.915658
import vitis

client = vitis.create_client()
client.set_workspace(path="tiny_sha3")

comp = client.get_component(name="hls_component")
comp.run(operation="IMPLEMENTATION")

