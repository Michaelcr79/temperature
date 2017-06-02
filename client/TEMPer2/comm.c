#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <usb.h>
#include <errno.h>

/*
 * Temper.c by Robert Kavaler (c) 2009 (relavak.com)
 * All rights reserved.
 *
 * Modified by Sylvain Leroux (c) 2012 (sylvain@chicoree.fr)
 *
 * Modified by Michael Robinson (c) 2017 (michael@robinson-west.com)
 *
 * Modified by Anthony Bateman (c) 2017 (tmbateman@hotmail.com)
 *
 * Temper driver for linux. This program can be compiled either as a library
 * or as a standalone program (-DUNIT_TEST). The driver will work with some
 * TEMPer usb devices from RDing (www.PCsensor.com).
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS ''AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL Robert kavaler BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include "comm.h"

/* #define debugit */

static int TEMPer2V13ToTemperature(Temper*,int16_t word, TemperData* dst);
static int TEMPerHUMToTemperature(Temper*,int16_t word, TemperData* dst);
static int TEMPerHUMToHumidity(Temper*,int16_t word, TemperData* dst);
static int TemperUnavailable(Temper*,int16_t word, TemperData* dst);

static const struct Product ProductList[] = 
{
	{
		/* Analog Device ADT75 (or similar) based device */
		/* with two temperature sensors (internal & external) */
		0x0c45, 0x7401,
		"RDing TEMPer2V1.3",
		{
			TEMPer2V13ToTemperature,
			TEMPer2V13ToTemperature,
		},
	},
	{
		/* Sensirion SHT1x based device */
		/* with internal humidity & temperature sensor */
		0x0c45, 0x7402,
		"RDing TEMPerHumiV1.1",
		{
			TEMPerHUMToTemperature,
			TEMPerHUMToHumidity,
		},
	},
};
static const unsigned ProductCount = sizeof(ProductList)/sizeof(struct Product);



Temper * TemperCreate(struct usb_device *dev, int timeout, int debug, 
                       const struct Product* product
                     )
{
#ifdef debugit
        printf("%s\n","TemperCreate entered...");
#endif

	Temper *t;
	int ret;

        // Create a Temper instance for Temper t. *****************************
	if (debug) 
        {
		printf("Temper device %s (%04x:%04x)\n",
			product->name,
			product->vendor,
			product->id
                      );
	}

	t = calloc(1, sizeof(*t)); // Use calloc to initialize memory to 0.
	t->device = dev;
	t->timeout = timeout;
	t->debug = debug;
	t->product = product;

	t->handle = usb_open(t->device);
	if(!t->handle) 
        {
		free(t);
		return NULL; // No handle created for product.
	}
        // Create a Temper instance for Temper t. *****************************

        /* I don't understand why I need to detach the Temper device from t.
         * There are two rounds of detachment here.
         */
        // Detach the kernel driver from Temper t. ****************************
	if(t->debug) 
        {
		printf("Trying to detach kernel driver\n");
	}

	ret = usb_detach_kernel_driver_np(t->handle, 0);
	if(ret) // Detach of kernel driver failed!
        {
		if(errno == ENODATA) 
                {
			if(t->debug) 
                        {
				printf("ENODATA: Device already detached 0\n");
			}
		} 
                else 
                {
			if(t->debug) 
                        {
				printf("Detach failed: %s[%d]\n",strerror(errno), errno);
				printf("Continuing anyway\n");
			}
		}
	} 
        else // Detach of kernel driver successful!
        {
		if(t->debug) 
                {
			printf("detach successful\n");
		}
	}

	ret = usb_detach_kernel_driver_np(t->handle, 1);
	if(ret) 
        {
		if(errno == ENODATA) 
                {
			if(t->debug)
				printf("ENODATA: Device already detached 1\n");
		} 
                else 
                {
			if(t->debug) 
                        {
				printf("Detach failed: %s[%d]\n",
				       strerror(errno), errno);
				printf("Continuing anyway\n");
			}
		}
	} 
        else 
        {
		if(t->debug) 
                {
			printf("detach successful\n");
		}
	}
        // Detach the kernel driver from Temper t. ****************************


        /* Something to do with can't get a handle on the device??? */
	if(usb_set_configuration(t->handle, 1) < 0 ||
	   usb_claim_interface(t->handle, 0) < 0 ||
	   usb_claim_interface(t->handle, 1)) 
        {
		usb_close(t->handle);
		free(t);
		return NULL; // No Temper device to return.
	}


	return t; // Return a Temper device.
}



/* Counter the number of Temper2 temperature sensors and Humidity sensors
 * and return the count.
 */
int TemperCount ()
{
#ifdef debugit
    printf ("%s\n","TemperCount entered...");
#endif

    struct usb_bus *bus;
    int number_tempers = 0; 

    for (bus=usb_get_busses(); bus; bus=bus->next)
    {
         struct usb_device * dev;
         for(dev=bus->devices; dev; dev=dev->next)
         {
		for(unsigned i = 0; i < ProductCount; ++i) 
                {
		      if(dev->descriptor.idVendor == ProductList[i].vendor &&
		         dev->descriptor.idProduct == ProductList[i].id) 
                      {
                         ++number_tempers;
                      }

                 } // Done checking each device against product list.

          } // Done stepping through all USB devices on a bus.

     } // Done stepping through all USB busses.

     return number_tempers;
}



Temper * TemperCreateFromDeviceNumber(int deviceNum, int timeout, int debug)
{
#ifdef debugit
        printf ("%s\n","TemperCreateFromDeviceNumber entered...");
#endif

	struct usb_bus *bus;
	int n;

	n = 0;
	for(bus=usb_get_busses(); bus; bus=bus->next) 
        {
	    struct usb_device *dev;

	    for(dev=bus->devices; dev; dev=dev->next) 
            {
		if(debug) 
                {
			printf("Found device: %04x:%04x\n",
			       dev->descriptor.idVendor,
			       dev->descriptor.idProduct);
		}
		for(unsigned i = 0; i < ProductCount; ++i) 
                {
			if(dev->descriptor.idVendor == ProductList[i].vendor 
                        && dev->descriptor.idProduct == ProductList[i].id) 
                        {
				if(debug) 
                                {
				    printf("Found deviceNum %d\n", n);
				}

				if(n == deviceNum) 
                                {
                                   if ( dev )
				   { return TemperCreate(dev, timeout,
	       			   		       debug,
						       &ProductList[i]
                                                      );
                                   }
				}

				n++;

                                // Put the current sensor in a list here.
			}

		} // Done looping through Temper products.

	    } // Done looping through all USB devices.

	} // Done looping through all USB busses.

	return NULL; // There aren't any TEMPer2 devices.
}


// Free the struct providing a handle for a TEMPer2 device. *******************
void TemperFree(Temper *t)
{
#ifdef debugit
        printf("%s\n","TemperFree entered...");
#endif

	if(t) 
        {
		if(t->handle) 
                {
			usb_close(t->handle);
		}
		free(t);
	}
}
// Free the struct providing a handle for a TEMPer2 device. *******************


// ****************************************************************************
// Below here is code that doesn't entirely make sense to me.
// ****************************************************************************


// This is for temperature sensors.
int TemperSendCommand8(Temper *t, 
                       int a, int b, int c, 
                       int d, int e, int f, 
                       int g, int h
                      )
{
#ifdef debugit
        printf("%s\n","TemperSendCommand8 entered...");
#endif

	unsigned char buf[8+8*8];
	int ret;

        // What is this next line doing???
	bzero(buf, sizeof(buf));
	buf[0] = a;
	buf[1] = b;
	buf[2] = c;
	buf[3] = d;
	buf[4] = e;
	buf[5] = f;
	buf[6] = g;
	buf[7] = h;

	if(t->debug) 
        {
           printf("sending bytes %02x, %02x, %02x, %02x, ", a, b, c, d);
           printf("%02x, %02x, %02x, %02x ", e, f, g, h);
           printf("(buffer len = %d)\n", sizeof(buf));
	}

	ret = usb_control_msg(t->handle, 0x21, 9, 0x200, 0x01,
			    (char *) buf, sizeof(buf), t->timeout);

	if(ret != sizeof(buf)) 
        {
		perror("usb_control_msg failed");
		return -1;
	}

	return 0;
}


// I assume this is for humidity sensors.
int TemperSendCommand2(Temper *t, int a, int b)
{
#ifdef debugit
        printf("%s\n","TemperSendCommand2 entered...");
#endif

	unsigned char buf[8+8*8];
	int ret;

	bzero(buf, sizeof(buf));
	buf[0] = a;
	buf[1] = b;

	if(t->debug) 
        {
		printf("sending bytes %02x, %02x (buffer len = %d)\n",
		       a, b, sizeof(buf));
	}

	ret = usb_control_msg(t->handle, 0x21, 9, 0x201, 0x00,
			    (char *) buf, sizeof(buf), t->timeout);

	if(ret != sizeof(buf)) 
        {
		perror("usb_control_msg failed");
		return -1;
	}

	return 0;
}



// Read from temperature sensor Temper into buffer and return status.
int TemperInterruptRead(Temper* t, unsigned char *buf, unsigned int len) 
{
#ifdef debugit
        printf("%s\n","TemperInterruptRead entered...");
#endif

	int ret;

#ifdef debugit
        printf("-----------------------------------------------------------\n");
        printf("%s%d\n","Size of buf:",sizeof(buf));
        printf("%s%d\n","Size of temper:",sizeof(t));
        printf("%s%d\n","Size of len:",len);
        printf("-----------------------------------------------------------\n");

        printf("%s\n","About to call usb_interrupt_read");
#endif

	ret = usb_interrupt_read(t->handle, 0x82, (char*)buf, len, t->timeout);
	if(t->debug) 
        {
		printf("receiving %d bytes\n",ret);
		for(int i = 0; i < ret; ++i) 
                {
			printf("%02x ", buf[i]);
			if ((i+1)%8 == 0) printf("\n");
		}
		printf("\n");
        }

	return ret;
}



// Where does the dst -> value equation come from???
static int TEMPer2V13ToTemperature(Temper* t, int16_t word, TemperData* dst) 
{
#ifdef debugit
       printf("%s\n","TEMPer2V13ToTemperature entered...");
#endif
#if 0
	word += t->offset; /* calibration value */
#endif

	dst->value = ((float)word) * (125.0 / 32000.0);
	dst->unit = TEMPER_ABS_TEMP;

	return 0;
}


// Where does the dst -> value equation come from???
static int TEMPerHUMToTemperature(Temper* t, int16_t word, TemperData* dst) 
{
#ifdef debugit
        printf("%s\n","TEMPerHUMToTemperature entered...");
#endif
#if 0
	word += t->offset; /* calibration value */
#endif

	/* assuming Vdd = 5V (from USB) and 14bit precision */
	dst->value = ((float)word) * (0.01) + -40.1;
	dst->unit = TEMPER_ABS_TEMP;

	return 0;
}


// Where does the dst -> value equation come from???
static int TEMPerHUMToHumidity(Temper* t, int16_t word, TemperData* dst) 
{
#ifdef debugit
        printf("%s\n","TEMPerHUMToHumidity entered...");
#endif

	const float rh = (float)word;

	/* assuming 12 bits readings */
	const float c1 = -2.0468;
	const float c2 = .0367;
	const float c3 = -1.5955e-6;

	dst->value = c1 + c2*rh + c3*rh*rh;
	dst->unit = TEMPER_REL_HUM;

	return 0;
}


// Sets the handle for unavailable, why???
static int TemperUnavailable(Temper* t, int16_t word, TemperData* dst) 
{
#ifdef debugit
        printf("%s\n","TemperUnavailable entered...");
#endif

	dst->value = 0.0;
	dst->unit = TEMPER_UNAVAILABLE;

	return 0;
}


// Bit shifting here to get the data???
int TemperGetData(Temper *t, struct TemperData *data, unsigned int count) 
{
#ifdef debugit
        printf("%s\n","TemperGetData entered...\n");
#endif

	unsigned char buf[8];
	int ret = TemperInterruptRead(t, buf, sizeof(buf));

	for(int i = 0; i < count; ++i) 
        {
		if ((2*i+3) < ret) 
                {
			int16_t word = ((int8_t)buf[2*i+2] << 8) | buf[2*i+3];
			t->product->convert[i](t, word, &data[i]);
		}
		else 
                {
			TemperUnavailable(t, 0, &data[i]);
		}

	} // End of reading loop.

	return ret;
}


// What serial number, what is it used for???
int TemperGetSerialNumber(Temper* t, char* buf, unsigned int len) 
{
#ifdef debugit
        printf("%s\n","TemperGetSerialNumber entered...");
#endif

	if (len == 0)
		return -EINVAL;

	if (t->device->descriptor.iSerialNumber == 0) 
        {
		buf[0] = 0;
		return -ENOENT;
	}

	return usb_get_string_simple(t->handle,
				     t->device->descriptor.iSerialNumber,
				     buf, len
                                    );
}
