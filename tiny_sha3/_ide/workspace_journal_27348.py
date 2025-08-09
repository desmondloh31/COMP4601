# 2025-08-10T00:32:33.685113800
import vitis

client = vitis.create_client()
client.set_workspace(path="tiny_sha3")

comp = client.get_component(name="hls_component")
comp.run(operation="C_SIMULATION")

comp.run(operation="SYNTHESIS")

comp.run(operation="C_SIMULATION")

comp.run(operation="IMPLEMENTATION")

vitis.dispose()

