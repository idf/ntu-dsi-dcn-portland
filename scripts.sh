function statsdiff() {
    diff statistics/Fat-tree.xml statistics/PortLand.xml
}

function run_portland() {
    ./waf --run scratch/PortLand
}
