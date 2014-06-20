PixelsDatasetLDAPSearch
=======================

Simple command-line utility written in C to access an LDAP directory to obtain dataset information.

The motivation for this was to centralise the configuration needed to run batch jobs on a cluster,
processing images from a large number of datasets. The main `genpixels` function is modified to make
an LDAP lookup to find datasets which need to be processed. Internally, the key `dsimgPath` is used to
find the list of images which should be processed by `genpixels` in the normal way.

This utility is basically `genpixels` modified to talk to an LDAP directory.
