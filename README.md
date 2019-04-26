# Kontron SMARC-sAL28 CPLD driver

This document is work in progress for now. It just lists the device tree
fragment.

## Device tree bindings

    &i2c0 {
        sl28cpld@4a {
            #address-cells = <1>;
            #size-cells = <0>;
            compatible = "kontron,sl28cpld";
            reg = <0x4a>;

            watchdog@4 {
                compatible = "kontron,sl28cpld-wdt";
                reg = <0x4>;
            };

            fan@b {
                compatible = "kontron,sl28cpld-fan";
                reg = <0xb>;
            };

            pwm0@c {
                compatible = "kontron,sl28cpld-pwm";
                reg = <0xc>;
            };

            pwm1@e {
                compatible = "kontron,sl28cpld-pwm";
                reg = <0xe>;
            };

            gpio0@10 {
                compatible = "kontron,sl28cpld-gpio";
                reg = <0x10>;
            };

            gpio1@13 {
                compatible = "kontron,sl28cpld-gpio";
                reg = <0x13>;
            };

            gpi0@16 {
                compatible = "kontron,sl28cpld-gpi";
                reg = <0x16>;
            };

            gpo0@17 {
                compatible = "kontron,sl28cpld-gpo";
                reg = <0x17>;
            };

            gpo1@18 {
                compatible = "kontron,sl28cpld-gpo";
                reg = <0x18>;
            };
        };
    };

