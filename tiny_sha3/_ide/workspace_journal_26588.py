# 2025-08-09T23:24:21.114550900
import vitis

client = vitis.create_client()
client.set_workspace(path="tiny_sha3")

comp = client.get_component(name="hls_component")
comp.run(operation="SYNTHESIS")

comp.run(operation="CO_SIMULATION")

vitis.dispose()

