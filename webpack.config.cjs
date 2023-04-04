const path = require('path')

module.exports = {
	target: 'node', 
	mode: 'production',
  module: {
    rules: [
      {
        test: /\.js$/,
        exclude: /node_modules/,
        use: {
          loader: 'babel-loader',
          options: {
            presets: ['@babel/preset-env'],
          },
        },
      },
    ],
  },
  resolve: {
    extensions: ['.js', '.mjs'],
  },
  experiments: {
    topLevelAwait: true,
  },
  //node: {
    //__dirname: true,
	  //__filename: true
  //},
  output: {
    filename: 'webpack.cjs',
    path: path.resolve(__dirname, 'dist'),
    libraryTarget: 'commonjs',
  },
};
