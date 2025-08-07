# 2025-08-08T02:13:44.774716
import vitis

client = vitis.create_client()
client.set_workspace(path="tiny_sha3")

comp = client.get_component(name="hls_component")
comp.run(operation="SYNTHESIS")

comp.run(operation="SYNTHESIS")

comp.run(operation="CO_SIMULATION")

comp.run(operation="IMPLEMENTATION")

