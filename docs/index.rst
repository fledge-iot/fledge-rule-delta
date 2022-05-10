.. Images
.. |delta_1| image:: images/delta_1.jpg
.. |delta_2| image:: images/delta_2.jpg
.. |delta_3| image:: images/delta_3.jpg
.. |delta_4| image:: images/delta_4.jpg

Moving Delta Rule
===================

The *fledge-rule-delta* plugin is a notifcation rule that is used to detect when a value moves outside of the determined delta by more than a specified percentage. The plugin only monitors a single asset, but will monitor all data points within that asset. It will trigger if any of the data points within the asset differ by more than the configured percentage, an delta is maintained for each data point separately.

During the configuration of a notification use the screen presented to choose the delta plugin as the rule.

+-------------+
| |delta_1| |
+-------------+

The next screen you are presented with provides the configuration options for the rule.

+-------------+
| |delta_2| |
+-------------+

The *Asset* entry field is used to define the single asset that the plugin should monitor.

The *Deviation %* defines how far away from the observed delta the current value should be in order to considered as triggering the rule.

+-------------+
| |delta_3| |
+-------------+

The *Direction* entry is used to define if the rule should trigger when the current value is above delta, below delta or in both cases.

+-------------+
| |delta_4| |
+-------------+

The *Delta* entry is used to determine what type of delta is used for the calculation. The delta calculated may be either a simple moving delta or an exponential moving delta. If an exponential moving delta is chosen then a second configuration parameter, *EMA Factor*, allows the setting of the factor used to calculate that delta.

Exponential moving deltas give more weight to the recent values compared to historical values. The smaller the EMA factor the more weight recent values carry. A value of 1 for *EMA Factor* will only consider the most recent value.

.. note::

   The Delta rule is not applicable to all data, only simple numeric values are considered and those values should not deviate with an delta of 0 or close to 0 if good results are required. Data points that deviate wildly are also not suitable for this plugin.

