# Qt WebAssembly Application

This directory contains the WebAssembly build of the Qt VM application.

## Files

- `index.html` - Main HTML file that loads the application
- `vm.js` - JavaScript loader and WebAssembly bootstrap
- `vm.wasm` - Compiled WebAssembly binary
- `server.py` - Simple Python HTTP server for testing

## Running the Application

### Option 1: Using the included Python server
```bash
cd out
python3 server.py
```
Then open http://localhost:8000 in your browser.

### Option 2: Using any HTTP server
```bash
cd out
python3 -m http.server 8000
# or
npx serve .
# or
php -S localhost:8000
```

### Option 3: Using Docker
```bash
docker run --rm -p 8000:8000 -v $(pwd):/usr/share/nginx/html nginx:alpine
```

## Browser Support

This application requires:
- Modern browser with WebAssembly support
- SharedArrayBuffer support (for better performance)
- No CORS restrictions (must be served via HTTP/HTTPS, not file://)

## Development

To rebuild the WebAssembly application:
```bash
docker buildx build . -f dist/web.Dockerfile --output out
```

## Troubleshooting

1. **Application doesn't load**: Check browser console for errors
2. **WASM loading failed**: Ensure both `vm.js` and `vm.wasm` are present
3. **Performance issues**: Enable SharedArrayBuffer headers (included in server.py)
4. **CORS errors**: Use a proper HTTP server, don't open files directly

## File Sizes

- `vm.js`: ~534KB (JavaScript loader)
- `vm.wasm`: ~19.8MB (WebAssembly binary)