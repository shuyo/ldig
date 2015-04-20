#!/usr/bin/env python2

from setuptools import setup

setup(
    name='ldig',
    version='0.1.0',
    url='https://github.com/shuyo/ldig',
    license='MIT',
    author='(c)2011-2012 Nakatani Shuyo / Cybozu Labs Inc. All rights reserved.',
    description='This is a prototype of language detection for short message'
    ' service (twitter). With 99.1% accuracy for 17 languages',
    packages=['ldig'],
    include_package_data=True,
    zip_safe=False,
    platforms='any',
    classifiers=[
        'Development Status :: 3 - Alpha',
        'Environment :: Console',
        'Environment :: Web Environment',
        'Intended Audience :: Developers',
        'Intended Audience :: Science/Research',
        'License :: OSI Approved :: MIT License',
        'Operating System :: OS Independent',
        'Programming Language :: Python',
        'Programming Language :: Python :: 2',
        'Topic :: Scientific/Engineering',
        'Topic :: Scientific/Engineering :: Information Analysis',
        'Topic :: Software Development',
        'Topic :: Software Development :: Internationalization',
        'Topic :: Software Development :: Libraries',
    ],
)
