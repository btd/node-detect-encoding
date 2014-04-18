{
  'targets': [
    {
      'target_name': 'detect_encoding',
      'sources': [ 'detect-encoding.cpp' ],
      'cflags!': [ '<!@(icu-config --cppflags)' ],
      'libraries': [ '<!@(icu-config --ldflags)' ],
      'conditions': [
        ['OS=="mac"', {
          'include_dirs': [
              '<!@(icu-config --prefix)/include'
          ]
        }]
      ]
    }
  ]
}
