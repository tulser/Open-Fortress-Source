This is a linux-only shim between Source & our client.so,
that solves linker issues.

gameinfo.txt specifies a GameBin that is "open_fortress/bin/ldproxy/",
which only contains our ldproxy shared object (client.so).
When it is loaded, it checks the /proc/{}/map
looking for itself, to find the path to open_fortress.
With its own path in hand, it loads all libraries listed in
`open_fortress/bin/ldproxy.txt`.
