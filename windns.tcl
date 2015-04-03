namespace eval windns {
    hook::add finload_hook \
	[namespace current]::init 90
}

proc windns::init {} {
    package require dns
    if {![catch {package require windns 0.1}]} {
	rename ::dns::nameservers ::dns::nameservers_old 
	rename ::windns::nameservers ::dns::nameservers	
    }
}
