import {defineConfig} from 'vite'
import vue from '@vitejs/plugin-vue'

export default defineConfig({
  root: 'src/admin',
  base: './',
  plugins: [vue()],
  build: {
    outDir: '../../build/http/admin'
  }
})
