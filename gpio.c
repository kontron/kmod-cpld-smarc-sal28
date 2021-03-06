/*
 * SMARC-sAL28 GPIO driver.
 *
 * Copyright 2019 Kontron Europe GmbH
 *
 * This file is licensed under  the terms of the GNU General Public
 * License version 2. This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_address.h>
#include <linux/regmap.h>
#include <linux/platform_device.h>
#include <linux/gpio/driver.h>
#include "sl28cpld.h"

/*
 * Watchdog timer block registers.
 */
#define SL28CPLD_GPIO_DIR	0
#define SL28CPLD_GPIO_OUT	1
#define SL28CPLD_GPIO_IN	2

enum sl28cpld_gpio_type {
	sl28cpld_gpio,
	sl28cpld_gpi,
	sl28cpld_gpo,
};

struct sl28cpld_gpio {
	struct gpio_chip gpio_chip;
	struct regmap *regmap;
	u32 offset;
};

static int sl28cpld_gpio_get_direction(struct gpio_chip *chip,
				       unsigned int offset)
{
	struct sl28cpld_gpio *gpio = gpiochip_get_data(chip);
	unsigned int reg;
	int ret;

	ret = regmap_read(gpio->regmap, gpio->offset + SL28CPLD_GPIO_DIR,
			  &reg);
	if (ret)
		return ret;

	return !(reg & (1 << offset));
}

static int __sl28cpld_gpio_direction(struct gpio_chip *chip,
				     unsigned int mask,
				     unsigned int val)
{
	struct sl28cpld_gpio *gpio = gpiochip_get_data(chip);

	return regmap_update_bits(gpio->regmap,
				  gpio->offset + SL28CPLD_GPIO_DIR,
				  mask, val);

}

static int sl28cpld_gpio_direction_input(struct gpio_chip *chip,
					 unsigned int offset)
{
	unsigned int mask = 1 << offset;
	return __sl28cpld_gpio_direction(chip, mask, 0);
}

static int sl28cpld_gpio_direction_output(struct gpio_chip *chip,
					  unsigned int offset, int value)
{
	unsigned int mask = 1 << offset;
	return __sl28cpld_gpio_direction(chip, mask, mask);
}

static int sl28cpld_gpi_direction_input(struct gpio_chip *chip,
					unsigned int offset)
{
	return 0;
}

static int __sl28cpld_gpio_get(struct gpio_chip *chip,
			       unsigned int offset, unsigned int addr)
{
	struct sl28cpld_gpio *gpio = gpiochip_get_data(chip);
	unsigned int reg;
	int ret;

	ret = regmap_read(gpio->regmap, gpio->offset + addr, &reg);
	if (ret)
		return ret;

	return !!(reg & (1 << offset));
}

static int sl28cpld_gpio_get(struct gpio_chip *chip, unsigned int offset)
{
	return __sl28cpld_gpio_get(chip, offset, SL28CPLD_GPIO_IN);
}

static int sl28cpld_gpi_get(struct gpio_chip *chip, unsigned int offset)
{
	return __sl28cpld_gpio_get(chip, offset, 0);
}

static void __sl28cpld_gpio_set(struct gpio_chip *chip,
				unsigned int offset, int value,
				unsigned int addr)
{
	struct sl28cpld_gpio *gpio = gpiochip_get_data(chip);
	int mask = 1 << offset;
	int val = value << offset;

	regmap_update_bits(gpio->regmap, gpio->offset + addr,
			   mask, val);
}

static void sl28cpld_gpio_set(struct gpio_chip *chip, unsigned int offset,
			      int value)
{
	__sl28cpld_gpio_set(chip, offset, value, SL28CPLD_GPIO_OUT);
}

static void sl28cpld_gpo_set(struct gpio_chip *chip, unsigned int offset,
			     int value)
{
	__sl28cpld_gpio_set(chip, offset, value, 0);
}

static const struct of_device_id sl28cpld_gpio_match[] = {
	{
		.compatible = "kontron,sl28cpld-gpio",
		.data = (void*)sl28cpld_gpio,
	},
	{
		.compatible = "kontron,sl28cpld-gpi",
		.data = (void*)sl28cpld_gpi,
	},
	{
		.compatible = "kontron,sl28cpld-gpo",
		.data = (void*)sl28cpld_gpo,
	},
	{}
};
MODULE_DEVICE_TABLE(of, sl28cpld_gpio_match);

static int sl28cpld_gpio_probe(struct platform_device *pdev)
{
	struct sl28cpld_gpio *gpio;
	const struct of_device_id *match;
	enum sl28cpld_gpio_type type;
	struct device *parent;
	struct gpio_chip *chip;
	int ret;
	const __be32 *reg;

	match = of_match_device(sl28cpld_gpio_match, &pdev->dev);
	if (!match)
		return -ENODEV;
	type = (enum sl28cpld_gpio_type)match->data;

	gpio = devm_kzalloc(&pdev->dev, sizeof(*gpio), GFP_KERNEL);
	if (!gpio)
		return -ENOMEM;

	parent = pdev->dev.parent;
	if (!parent) {
		dev_err(&pdev->dev, "No parent for sl28cpld-gpio\n");
		return -ENODEV;
	}

	gpio->regmap = sl28cpld_node_to_regmap(parent->of_node);
	if (IS_ERR(gpio->regmap)) {
		dev_err(&pdev->dev, "No regmap for parent\n");
		return PTR_ERR(gpio->regmap);
	}

	reg = of_get_address(pdev->dev.of_node, 0, NULL, NULL);
	if (!reg) {
		dev_err(&pdev->dev, "No 'offset' missing\n");
		return -EINVAL;
	}
	gpio->offset = be32_to_cpu(*reg);

	/* initialize struct gpio_chip */
	chip = &gpio->gpio_chip;
	chip->parent = &pdev->dev;
	chip->label = dev_name(&pdev->dev);
	chip->owner = THIS_MODULE;
	chip->can_sleep = true;
	chip->base = -1;
	chip->ngpio = 8;

	switch (type) {
	case sl28cpld_gpio:
		chip->get_direction = sl28cpld_gpio_get_direction;
		chip->direction_input = sl28cpld_gpio_direction_input;
		chip->direction_output = sl28cpld_gpio_direction_output;
		chip->get = sl28cpld_gpio_get;
		chip->set = sl28cpld_gpio_set;
		break;
	case sl28cpld_gpo:
		chip->set = sl28cpld_gpo_set;
		chip->get = sl28cpld_gpi_get;
		break;
	case sl28cpld_gpi:
		chip->direction_input = sl28cpld_gpi_direction_input;
		chip->get = sl28cpld_gpi_get;
		break;
	}

	ret = devm_gpiochip_add_data(&pdev->dev, &gpio->gpio_chip, gpio);
	if (ret < 0)
		return ret;

	platform_set_drvdata(pdev, gpio);

	return 0;
}

static struct platform_driver sl28cpld_gpio_driver = {
	.probe		= sl28cpld_gpio_probe,
	.driver = {
		.name = "sl28cpld-gpio",
		.of_match_table = of_match_ptr(sl28cpld_gpio_match),
	},
};
module_platform_driver(sl28cpld_gpio_driver);

MODULE_AUTHOR("Michael Walle <michael.walle@kontron.com>");
MODULE_DESCRIPTION("sl28 CPLD GPIO Driver");
MODULE_LICENSE("GPL");
