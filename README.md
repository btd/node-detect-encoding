# Detect encoding name with ICU

## Installation

Install `libicu`.

In your `$PATH` `icu-config` should be available.

    npm install detect-encoding

#### OSX

* Brew

```
brew install icu4c
export PATH=$PATH:/usr/local/Cellar/icu4c/52.1 #take a look which version you have
```

## Usage

### Simple usage

```javascript
var detectEncoding = require("detect-encoding");

var buffer = fs.readFileSync("/path/to/the/file");
var charset = detectEncoding(buffer, function(err, result) {
    console.log(result)
});
```
