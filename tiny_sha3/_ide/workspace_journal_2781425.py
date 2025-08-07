# 2025-08-06T17:13:56.003880
import vitis

client = vitis.create_client()
client.set_workspace(path="tiny_sha3")

comp = client.create_hls_component(name = "hls_component",cfg_file = ["hls_config.cfg"],template = "empty_hls_component")

comp = client.get_component(name="hls_component")
comp.run(operation="SYNTHESIS")

