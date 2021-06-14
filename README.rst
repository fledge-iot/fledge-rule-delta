========================================
Fledge Delta notification rule plugin
========================================

A notification plugin that triggers if the value of a datapoint is
more than a prescribed percentage different from the currently observed
delta for that data point.

The plugin only monitors a single asset, but will moitor all data points
within that asset. It will trigger if any of the data points within the
asset differ by more than the configured percentage, an delta is maintained
for each data point seperately.

A configuration option also allows for control of notification triggering
based on the value being above, below are either side of the delta
value.

The delta calculated may be either a simple moving delta or an
exponential moving delta. If an exponential moving delta is chosen
then a second configuration parameter allows the setting of the factor
used to calculate that delta.

Exponential moving deltas give more weight to the recent values compare
to historical values. The smaller the EMA factor the more weight recent
values carry. A value of 1 for factor will only consider at the most recent
value.

The Delta rule is not applicable to all data, only simple numeric values
are considered and those values should not deviate with an delta of
0 or close to 0 if good results are required. Data points that deviate
wildly are also not suitable for this plugin.

Build
-----
To build Fledge "Delta" notification rule C++ plugin,
in addition to Fledge source code, the Notification server C++
header files are required (no .cpp files or libraries needed so far)

The path with Notification server C++ header files cab be specified only via
NOTIFICATION_SERVICE_INCLUDE_DIRS environment variable.

Example:

.. code-block:: console

  $ export NOTIFICATION_SERVICE_INCLUDE_DIRS=/home/ubuntu/source/fledge-service-notification/C/services/common/include

.. code-block:: console

  $ mkdir build
  $ cd build
  $ cmake ..
  $ make

- By default the Fledge develop package header files and libraries
  are expected to be located in /usr/include/fledge and /usr/lib/fledge
- If **FLEDGE_ROOT** env var is set and no -D options are set,
  the header files and libraries paths are pulled from the ones under the
  FLEDGE_ROOT directory.
  Please note that you must first run 'make' in the FLEDGE_ROOT directory.

You may also pass one or more of the following options to cmake to override 
this default behaviour:

- **FLEDGE_SRC** sets the path of a Fledge source tree
- **FLEDGE_INCLUDE** sets the path to Fledge header files
- **FLEDGE_LIB sets** the path to Fledge libraries
- **FLEDGE_INSTALL** sets the installation path of Random plugin

NOTE:
 - The **FLEDGE_INCLUDE** option should point to a location where all the Fledge 
   header files have been installed in a single directory.
 - The **FLEDGE_LIB** option should point to a location where all the Fledge
   libraries have been installed in a single directory.
 - 'make install' target is defined only when **FLEDGE_INSTALL** is set

Examples:

- no options

  $ cmake ..

- no options and FLEDGE_ROOT set

  $ export FLEDGE_ROOT=/some_fledge_setup

  $ cmake ..

- set FLEDGE_SRC

  $ cmake -DFLEDGE_SRC=/home/source/develop/Fledge  ..

- set FLEDGE_INCLUDE

  $ cmake -DFLEDGE_INCLUDE=/dev-package/include ..
- set FLEDGE_LIB

  $ cmake -DFLEDGE_LIB=/home/dev/package/lib ..
- set FLEDGE_INSTALL

  $ cmake -DFLEDGE_INSTALL=/home/source/develop/Fledge ..

  $ cmake -DFLEDGE_INSTALL=/usr/local/fledge ..
