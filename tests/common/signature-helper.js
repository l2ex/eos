const BN = require('bn.js')
const Eos = require('eosjs')
const ecc = require('eosjs-ecc')


class SignatureHelper {

    // Serializating/deserializating message

    static serializeMessage(channelOwner, symbol, change, nonce, apply) {
        // TODO: Should it be optimized?
        const asset = `${change} ${symbol}`
        const channelOwnerString = Eos.modules.format.encodeName(channelOwner, false)
        const symbolString = this.symbolToNumber(Eos.modules.format.parseAsset(asset).precision, symbol)
        const changeString = change.replace('.', '')
        const nonceString = nonce.toString()
        const applyString = apply ? '1' : '0'
        const message =
            channelOwnerString + ';' +
            symbolString + ';' +
            changeString + ';' +
            nonceString + ';' +
            applyString;
        return message
    }

    static deserializeMessage(message) {
        // TODO: Should it be optimized?
        return {
            channelOwner: '',
            symbol: '',
            change: '',
            nonce: '',
            apply: ''
        }
    }

    // Signing/recovering message

    // data - data to hash as Buffer
    // returns 32 bytes hash as Buffer
    static sha256(data) {
        return ecc.sha256(data, null)
    }

    // messageHash - sha256 hash of signing message as Buffer
    // privateKey - private key used to sign message hash as String
    static sign(messageHash, privateKey) {
        return ecc.signHash(messageHash, privateKey)
    }

    // messageHash - sha256 hash of signing message as Buffer
    // signature - signature as Buffer?
    static recover(messageHash, signature) {
        return ecc.recoverHash(signature, messageHash)
    }

    // Helper functions

    static symbolToNumber(precision, symbol) {
        var result = new BN(0)
        for (var i = 0; i < symbol.length; ++i) {
            if (symbol[i] >= 'A' && symbol[i] <= 'Z') {
                result = result.or(new BN(symbol.charCodeAt(i)).shln(8 * (1 + i)))
            }
        }
        result = result.or(new BN(precision))
        return result
    }
    /*

   static constexpr uint64_t string_to_symbol( uint8_t precision, const char* str ) {
      uint32_t len = 0;
      while( str[len] ) ++len;

      uint64_t result = 0;
      for( uint32_t i = 0; i < len; ++i ) {
         if( str[i] < 'A' || str[i] > 'Z' ) {
            /// ERRORS?
         } else {
            result |= (uint64_t(str[i]) << (8*(1+i)));
         }
      }

      result |= uint64_t(precision);
      return result;
   }
   */
}

module.exports = SignatureHelper
